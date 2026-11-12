#!/bin/bash
# P0 pre-flight: write "passwd 1234" into the startup configuration and prove
# that (a) the read-back contains it, (b) the baseline is intact and (c) the
# password already works on the *running* firmware before the new image is
# flashed. If any of that fails the device must not be flashed: the patched
# firmware locks the web interface when no password is configured, and there is
# no serial console attached to recover it.
#
# usage: p0-seed.sh <ip> <port> <expected-mac> [password]
set -u

IP=$1; PORT=$2; MAC=$3; PW=${4:-1234}
BASE="http://$IP:$PORT"
JAR="/tmp/p0-jar-$IP"
LOG="upgrade/logs/p0-seed-$IP.log"
mkdir -p upgrade/logs

log() { echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }
die() { log "ABORT: $*"; exit 1; }

log "=== seed $IP:$PORT (expect $MAC)"

curl -sS -m 5 "$BASE/information.json" -o "/tmp/p0-info-$IP.json" || die "information.json"
grep -q "\"mac_address\":\"$MAC\"" "/tmp/p0-info-$IP.json" || die "MAC mismatch: $(cat "/tmp/p0-info-$IP.json")"

curl -sS -m 5 "$BASE/config" -o "/tmp/p0-cfg-before-$IP.txt" || die "GET /config"
grep -q '^ip ' "/tmp/p0-cfg-before-$IP.txt" || die "config looks empty: $(cat "/tmp/p0-cfg-before-$IP.txt")"
if grep -q '^passwd ' "/tmp/p0-cfg-before-$IP.txt"; then
	die "config already has a passwd line, refusing to overwrite: $(grep '^passwd ' "/tmp/p0-cfg-before-$IP.txt")"
fi

# Keep the exact baseline, only append the password line.
cp "/tmp/p0-cfg-before-$IP.txt" "/tmp/p0-cfg-new-$IP.txt"
[ -n "$(tail -c1 "/tmp/p0-cfg-new-$IP.txt")" ] && echo >> "/tmp/p0-cfg-new-$IP.txt"
echo "passwd $PW" >> "/tmp/p0-cfg-new-$IP.txt"

RESP=$(curl -sS -m 20 -F "configuration=@/tmp/p0-cfg-new-$IP.txt;type=application/octet-stream;filename=config.txt" -w '\n%{http_code}' "$BASE/config" 2>&1)
CODE=$(echo "$RESP" | tail -1)
log "config write http=$CODE $(echo "$RESP" | head -1 | cut -c1-80)"
[ "$CODE" = "200" ] || die "config write failed"

sleep 1
curl -sS -m 5 "$BASE/config" -o "/tmp/p0-cfg-after-$IP.txt" || die "GET /config after write"
grep -qx "passwd $PW" "/tmp/p0-cfg-after-$IP.txt" || die "read-back has no passwd line: $(cat "/tmp/p0-cfg-after-$IP.txt")"

python3 - "$IP" "$PW" <<'PY' || exit 1
import sys
ip, pw = sys.argv[1], sys.argv[2]
def lines(p):
    return [l.rstrip("\r\n") for l in open(p, encoding="utf-8", errors="replace") if l.strip()]
before = lines(f"/tmp/p0-cfg-before-{ip}.txt")
after = lines(f"/tmp/p0-cfg-after-{ip}.txt")
missing = [l for l in before if l not in after]
if missing:
    print("ABORT: baseline lines lost:", missing); sys.exit(1)
extra = [l for l in after if l not in before]
print("  baseline kept:", len(before), "lines; added:", extra)
if extra != [f"passwd {pw}"]:
    print("ABORT: unexpected extra lines"); sys.exit(1)
PY
[ $? -eq 0 ] || die "read-back mismatch"

# The password must already work on the old firmware: this is the login the
# update script will reuse after the reboot.
rm -f "$JAR"
LOGIN=$(curl -sS -m 10 -o /dev/null -w '%{http_code}' -c "$JAR" -d "pwd=$PW" "$BASE/login" 2>&1)
log "login with $PW: http=$LOGIN"
[ "$LOGIN" = "302" ] || die "login with the new password failed"

WITH=$(curl -sS -m 5 -b "$JAR" -o /tmp/p0-cfg-auth-$IP.txt -w '%{http_code}' "$BASE/config")
log "authenticated /config: http=$WITH ($(stat -c%s /tmp/p0-cfg-auth-$IP.txt) bytes)"
[ "$WITH" = "200" ] || die "authenticated config read failed"
grep -qx "passwd $PW" "/tmp/p0-cfg-auth-$IP.txt" || die "authenticated view has no passwd line"

cp "/tmp/p0-cfg-before-$IP.txt" "upgrade/logs/p0-config-before-$IP.txt" 2>/dev/null || true
cp "/tmp/p0-cfg-after-$IP.txt" "upgrade/logs/p0-config-seeded-$IP.txt"
log "=== SEEDED $IP"
