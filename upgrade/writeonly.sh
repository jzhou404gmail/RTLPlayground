#!/bin/bash
# Write a startup configuration and reboot, without waiting for a specific IP
# (for a device that switches from static to DHCP and comes back elsewhere).
# usage: writeonly.sh <ip> <port> <config-file> <expected-mac>
set -u

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
	curl -sS -m 5 -o /dev/null -c "$JAR" -d "pwd=1234" "$BASE/login" 2>/dev/null || return 1
	curl -sS -m 5 -b "$JAR" "$BASE/information.json" 2>/dev/null
}

log "=== writing config to $IP:$PORT from $(basename "$CFG") (expect $MAC)"
INFO=$(getinfo)
if ! echo "$INFO" | grep -q "\"mac_address\":\"$MAC\""; then
	log "ABORT: device does not report MAC $MAC: $INFO"
	exit 1
fi

RESP=$(curl -sS -m 15 -b "$JAR" -F "configuration=@$CFG;type=application/octet-stream;filename=config.txt" -w '\n%{http_code}' "$BASE/config" 2>&1)
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
echo "$BACK" > "upgrade/logs/config-written-$IP.txt"

curl -sS -m 5 -b "$JAR" "$BASE/reset" >/dev/null 2>&1
log "reset requested"
exit 0
