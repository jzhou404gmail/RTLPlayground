#!/bin/bash
# Migrate one switch to the digest login and v0.1.7.
# usage: migrate-v017.sh <ip> <port> <mac> <model> <jedec> <image>
set -u
HERE=$(dirname "$0")
source "$HERE/authlib.sh"

IP=$1; PORT=$2; MAC=$3; MODEL=$4; JEDEC=$5; IMG=$6
B="http://$IP:$PORT"
J="$HERE/logs/.mig-$IP"
CFG="$HERE/logs/.mig-$IP.txt"
LOG="$HERE/logs/v017-migrate-$IP.log"
mkdir -p "$HERE/logs"
log() { echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }
die() { log "ABORT: $*"; exit 1; }

log "=== $IP -> v0.1.7"

# 1. log in with the old plaintext password (auth.json is absent on v0.1.4)
rm -f "$J"
auth_login "$B" "$J" 1234
INFO=$(curl -sS -m 5 -b "$J" "$B/information.json") || die "no information.json"
echo "$INFO" | grep -q "\"mac_address\":\"$MAC\"" || die "MAC mismatch: $INFO"
log "before: $(echo "$INFO" | sed -nE 's/.*"sw_ver":"([^"]+)".*/\1/p')"

# 2. append passwd_hash for 1234, keep the plaintext line as a fallback
curl -sS -m 5 -b "$J" "$B/config" -o "$CFG" || die "GET /config"
if grep -q '^passwd_hash ' "$CFG"; then
	log "passwd_hash already present, not touching the config"
else
	SALT=$(python3 -c 'import secrets;print(secrets.token_hex(16))')
	HASH=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(bytes.fromhex(sys.argv[1])+b"1234").hexdigest())' "$SALT")
	printf 'passwd_hash %s %s\n' "$SALT" "$HASH" >> "$CFG"
	CODE=$(curl -sS -m 20 -b "$J" -H 'X-RTLP: 1' -F "configuration=@$CFG;type=application/octet-stream;filename=config.txt" -o /dev/null -w '%{http_code}' "$B/config")
	log "config write http=$CODE"
	[ "$CODE" = "200" ] || die "config write failed"
	curl -sS -m 5 -b "$J" "$B/config" | grep -q "^passwd_hash $SALT $HASH$" || die "hash missing from read-back"
fi
rm -f "$J"

# 3. flash
"$HERE/flash_checked.sh" "$IP" "$PORT" "$IMG" "$MAC" "$MODEL" "$JEDEC" v0.1.7 || die "flash failed"

# 4. digest login, then drop the plaintext line
rm -f "$J"
auth_login "$B" "$J" 1234
curl -sS -m 5 -b "$J" "$B/config" -o "$CFG" || die "GET /config after flash"
if grep -q '^passwd ' "$CFG"; then
	python3 - "$CFG" <<'PY'
import sys
p=sys.argv[1]
lines=[l for l in open(p).read().splitlines() if l.strip() and not l.startswith('passwd ')]
open(p,'w').write('\n'.join(lines)+'\n')
PY
	CODE=$(curl -sS -m 20 -b "$J" -H 'X-RTLP: 1' -F "configuration=@$CFG;type=application/octet-stream;filename=config.txt" -o /dev/null -w '%{http_code}' "$B/config")
	log "plaintext removal http=$CODE"
	[ "$CODE" = "200" ] || die "plaintext removal failed"
	curl -sS -m 5 -b "$J" "$B/config" | grep -q '^passwd ' && die "plaintext still present"
	log "plaintext removed"
else
	log "no plaintext line left"
fi
rm -f "$J"

# 5. reboot and prove the digest-only login. A fresh login: the jar used for
# the config write was removed above.
rm -f "$J"
auth_login "$B" "$J" 1234
[ -s "$J" ] || die "login before reboot failed"

# Wait for a real downtime first:
# auth.json answers 200 immediately on a device that did not reset.
REBOOTED=0
for attempt in 1 2 3; do
	curl -sS -m 3 -b "$J" "$B/reset" -o /dev/null 2>/dev/null || true
	DOWN=0
	for i in $(seq 1 40); do
		sleep 1
		c=$(curl -s -o /dev/null -m 2 -w '%{http_code}' "$B/auth.json" 2>/dev/null)
		if [ "$c" != "200" ]; then DOWN=1; fi
		if [ "$DOWN" = 1 ] && [ "$c" = "200" ]; then
			REBOOTED=1
			log "rebooted after ~${i}s"
			break
		fi
	done
	[ "$REBOOTED" = 1 ] && break
	log "no reboot observed, retrying reset (attempt $attempt)"
done
[ "$REBOOTED" = 1 ] || die "device did not reboot"

AJ=$(curl -sS -m 5 "$B/auth.json")
echo "$AJ" | grep -q '"set":1' || die "auth.json: $AJ"
rm -f "$J"
auth_login "$B" "$J" 1234
CODE=$(curl -s -o /dev/null -m 5 -b "$J" -w '%{http_code}' "$B/config")
log "digest login /config: $CODE"
[ "$CODE" = "200" ] || die "digest login failed"
curl -sS -m 5 -b "$J" "$B/config" | grep -q '^passwd ' && die "plaintext came back"
PLAIN=$(curl -s -D- -o /dev/null -m 5 -d 'pwd=1234' "$B/login" | grep -ci '^set-cookie')
log "plaintext 1234 cookies: $PLAIN (expect 0)"
[ "$PLAIN" = "0" ] || die "plaintext login still accepted"
log "=== OK $IP"
rm -f "$J"
