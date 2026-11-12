#!/bin/bash
# Migrate a v0.1.1 switch (no password) to v0.1.7 with digest login.
# usage: migrate-new.sh <ip> <old-port> <new-port> <mac> <model> <hw> <jedec> <image>
set -u
HERE=$(dirname "$0")
source "$HERE/authlib.sh"

IP=$1; OLD=$2; NEW=$3; MAC=$4; MODEL=$5; HW=$6; JEDEC=$7; IMG=$8
O="http://$IP:$OLD"; N="http://$IP:$NEW"
J="$HERE/logs/.new-$IP"; CFG="$HERE/logs/.new-$IP.txt"
LOG="$HERE/logs/v017-new-$IP.log"
mkdir -p "$HERE/logs"
log() { echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }
die() { log "ABORT: $*"; exit 1; }

log "=== $IP:$OLD -> v0.1.7 (new port $NEW)"
strings -a "$IMG" | grep -q "$HW" || die "image does not carry $HW"
[ "$(stat -c%s "$IMG")" = "524288" ] || die "image size"

INFO=$(curl -sS -m 5 "$O/information.json") || die "no information.json on $OLD"
echo "$INFO" | grep -q "\"mac_address\":\"$MAC\"" || die "MAC mismatch: $INFO"
echo "$INFO" | grep -q "\"model\":\"$MODEL\"" || die "model mismatch: $INFO"
echo "$INFO" | grep -q "\"hw_ver\":\"$HW\"" || die "hw mismatch: $INFO"
log "before: $(echo "$INFO" | sed -nE 's/.*"sw_ver":"([^"]+)".*/\1/p') port $OLD"

JOUT=$(curl -sS -m 5 -X POST "$O/cmd" --data 'flash j') || die "flash j"
MID=$(echo "$JOUT" | sed -nE 's/.*Manufacturer ID:[[:space:]]*0x([0-9a-fA-F]+).*/\1/p' | head -1)
MT=$(echo "$JOUT" | sed -nE 's/.*Memory Type:[[:space:]]*0x([0-9a-fA-F]+).*/\1/p' | head -1)
CAP=$(echo "$JOUT" | sed -nE 's/.*Capacity:[[:space:]]*0x([0-9a-fA-F]+).*/\1/p' | head -1)
GOT=$(printf '%s%s%s' "$MID" "$MT" "$CAP" | tr 'A-F' 'a-f')
log "flash JEDEC: ${GOT:-unreadable} (expect $JEDEC)"
[ "$GOT" = "$JEDEC" ] || die "flash JEDEC mismatch"

curl -sS -m 5 "$O/config" -o "$CFG" || die "GET /config"
SALT=$(python3 -c 'import secrets;print(secrets.token_hex(16))')
HASH=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(bytes.fromhex(sys.argv[1])+b"1234").hexdigest())' "$SALT")
python3 - "$CFG" "$NEW" "$SALT" "$HASH" <<'PY'
import sys
p, port, salt, h = sys.argv[1:5]
lines=[l for l in open(p).read().splitlines() if l.strip()]
lines=[l for l in lines if not l.startswith(('http port ', 'passwd ', 'passwd_hash '))]
lines.append(f'http port {port}')
lines.append('passwd 1234')
lines.append(f'passwd_hash {salt} {h}')
open(p,'w').write('\n'.join(lines)+'\n')
PY
log "new config:"; sed 's/^/    /' "$CFG" | tee -a "$LOG"
CODE=$(curl -sS -m 20 -F "configuration=@$CFG;type=application/octet-stream;filename=config.txt" -o /dev/null -w '%{http_code}' "$O/config")
log "config write http=$CODE (port $OLD)"
[ "$CODE" = "200" ] || die "config write failed"
curl -sS -m 5 "$O/config" | grep -q "^passwd_hash $SALT $HASH$" || die "hash missing from read-back"

log "uploading $(basename "$IMG") ..."
RESP=$(curl -sS -m 300 -F "uploadedfile=@$IMG;type=application/octet-stream;filename=$(basename "$IMG")" "$O/upload" 2>&1)
echo "$RESP" | tee -a "$LOG"
echo "$RESP" | grep -q "checksum verified" || die "upload was not verified"

log "waiting for new firmware on port $NEW ..."
OK=0
for i in $(seq 1 80); do
	sleep 3
	c=$(curl -s -o /dev/null -m 2 -w '%{http_code}' "$N/auth.json" 2>/dev/null)
	if [ "$c" = "200" ]; then OK=1; log "back after ~$((i*3))s"; break; fi
done
[ "$OK" = 1 ] || die "new firmware did not come up on $NEW"

AJ=$(curl -sS -m 5 "$N/auth.json")
echo "$AJ" | grep -q '"set":1' || die "auth.json: $AJ"
rm -f "$J"
auth_login "$N" "$J" 1234
INFO=$(curl -sS -m 5 -b "$J" "$N/information.json") || die "digest login failed"
echo "$INFO" | grep -q '"sw_ver":"v0.1.7' || die "unexpected version: $INFO"
log "after: $(echo "$INFO" | sed -nE 's/.*"sw_ver":"([^"]+)".*/\1/p') port $(echo "$INFO" | sed -nE 's/.*"http_port":([0-9]+).*/\1/p')"

# remove the plaintext fallback and reboot
curl -sS -m 5 -b "$J" "$N/config" -o "$CFG" || die "GET /config after flash"
python3 - "$CFG" <<'PY'
import sys
p=sys.argv[1]
lines=[l for l in open(p).read().splitlines() if l.strip() and not l.startswith('passwd ')]
open(p,'w').write('\n'.join(lines)+'\n')
PY
CODE=$(curl -sS -m 20 -b "$J" -H 'X-RTLP: 1' -F "configuration=@$CFG;type=application/octet-stream;filename=config.txt" -o /dev/null -w '%{http_code}' "$N/config")
log "plaintext removal http=$CODE"
[ "$CODE" = "200" ] || die "plaintext removal failed"
curl -sS -m 5 -b "$J" "$N/config" | grep -q '^passwd ' && die "plaintext still present"
rm -f "$J"

REBOOTED=0
for attempt in 1 2 3; do
	auth_login "$N" "$J" 1234
	curl -sS -m 3 -b "$J" "$N/reset" -o /dev/null 2>/dev/null || true
	DOWN=0
	for i in $(seq 1 40); do
		sleep 1
		c=$(curl -s -o /dev/null -m 2 -w '%{http_code}' "$N/auth.json" 2>/dev/null)
		if [ "$c" != "200" ]; then DOWN=1; fi
		if [ "$DOWN" = 1 ] && [ "$c" = "200" ]; then REBOOTED=1; log "rebooted after ~${i}s"; break; fi
	done
	[ "$REBOOTED" = 1 ] && break
	log "no reboot observed, retrying"
done
[ "$REBOOTED" = 1 ] || die "device did not reboot"

rm -f "$J"
auth_login "$N" "$J" 1234
CODE=$(curl -s -o /dev/null -m 5 -b "$J" -w '%{http_code}' "$N/config")
log "digest login /config: $CODE"
[ "$CODE" = "200" ] || die "digest login failed"
curl -sS -m 5 -b "$J" "$N/config" | grep -q '^passwd ' && die "plaintext came back"
PLAIN=$(curl -s -D- -o /dev/null -m 5 -d 'pwd=1234' "$N/login" | grep -ci '^set-cookie')
log "plaintext 1234 cookies: $PLAIN (expect 0)"
[ "$PLAIN" = "0" ] || die "plaintext login still accepted"
rm -f "$J"
log "=== OK $IP (port $NEW, salt $SALT)"
