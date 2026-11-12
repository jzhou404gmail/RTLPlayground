#!/bin/bash
# Write the startup configuration over the web interface and reboot.
# usage: setconfig.sh <ip> <port> <config-file> <expected-mac>
set -u
source "$(dirname "$0")/authlib.sh"

IP=$1; PORT=$2; CFG=$3; MAC=$4
BASE="http://$IP:$PORT"
JAR="/tmp/dsh-jar-$IP"
LOG="upgrade/logs/config-$IP.log"
mkdir -p upgrade/logs

log() { echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }

getinfo() {
	local out
	out=$(curl -sS -m 5 "$BASE/information.json" 2>/dev/null || true)
	case "$out" in
	*'"mac_address"'*) echo "$out"; return 0 ;;
	esac
	rm -f "$JAR"
	auth_login "$BASE" "$JAR" 1234 || return 1
	curl -sS -m 5 -b "$JAR" "$BASE/information.json" 2>/dev/null
}

log "=== writing config to $IP:$PORT from $(basename "$CFG") (expect $MAC)"
INFO=$(getinfo)
if ! echo "$INFO" | grep -q "\"mac_address\":\"$MAC\""; then
	log "ABORT: device does not report MAC $MAC: $INFO"
	exit 1
fi

RESP=$(curl -sS -m 15 -b "$JAR" -H "X-RTLP: 1" -F "configuration=@$CFG;type=application/octet-stream;filename=config.txt" -w '\n%{http_code}' "$BASE/config" 2>&1)
log "config write: $(echo "$RESP" | tr '\n' ' ')"

BACK=$(curl -sS -m 15 -b "$JAR" "$BASE/config" 2>/dev/null)
if ! echo "$BACK" | grep -q "^ip dhcp$\|^ip 192\.168\."; then
	log "ABORT: read-back does not contain an ip line: $BACK"
	exit 1
fi
if echo "$BACK" | grep -q "^passwd "; then
	log "ABORT: password line still present: $BACK"
	exit 1
fi
log "read-back OK"

curl -sS -m 5 -b "$JAR" "$BASE/reset" >/dev/null 2>&1
log "reset requested; waiting ..."
OK=0
for i in $(seq 1 60); do
	sleep 3
	INFO=$(getinfo)
	if echo "$INFO" | grep -q "\"mac_address\":\"$MAC\""; then
		if ! curl -sS -m 5 -o /dev/null -w '%{http_code}' "$BASE/information.json" | grep -q 401; then
			log "back after ~$((i*3))s, no login needed"
			OK=1
			break
		fi
	fi
done
if [ $OK -ne 1 ]; then
	log "FAILED: device did not return with open access"
	exit 1
fi
echo "$INFO" > "upgrade/logs/info-final-$IP.json"
log "=== OK $IP"
exit 0
