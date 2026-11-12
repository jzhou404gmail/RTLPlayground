#!/bin/bash
# Flash one RTLPlayground image over the web updater with an explicitly given
# password (boards whose web password is not 1234), then wait for the reboot.
# usage: flash-experiment.sh <ip> <port> <image> <expected-mac> <password> [version-substring]
set -u
source "$(dirname "$0")/authlib.sh"

IP=$1; PORT=$2; IMG=$3; MAC=$4; PW=$5; WANT=${6:-}; PW_AFTER=${7:-1234}
BASE="http://$IP:$PORT"
JAR="/tmp/exp-jar-$IP"
LOG="upgrade/logs/flash-experiment-$IP.log"
mkdir -p upgrade/logs

log() { echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }

# $1: the password to log in with (the image may embed a different one)
getinfo() {
	rm -f "$JAR"
	auth_login "$BASE" "$JAR" "$1" || return 1
	curl -sS -m 5 -b "$JAR" "$BASE/information.json" 2>/dev/null
}

log "=== flashing $IP:$PORT with $(basename "$IMG") (expect $MAC)"
INFO=$(getinfo "$PW")
if ! echo "$INFO" | grep -q "\"mac_address\":\"$MAC\""; then
	log "ABORT: device does not report MAC $MAC: $INFO"
	exit 1
fi
BEFORE=$(echo "$INFO" | sed -E 's/.*"sw_ver":"([^"]+)".*/\1/')
log "before: $BEFORE"

RESP=$(curl -sS -m 300 -b "$JAR" -H "X-RTLP: 1" \
	-F "uploadedfile=@$IMG;type=application/octet-stream;filename=$(basename "$IMG")" \
	"$BASE/upload" 2>&1)
RC=$?
log "upload rc=$RC response: $RESP"
if ! echo "$RESP" | grep -q "checksum verified"; then
	log "ABORT: upload did not verify"
	exit 1
fi

log "waiting for reboot ..."
OK=0
for i in $(seq 1 80); do
	sleep 3
	INFO=$(getinfo "$PW_AFTER")
	if echo "$INFO" | grep -q "\"mac_address\":\"$MAC\""; then
		AFTER=$(echo "$INFO" | sed -E 's/.*"sw_ver":"([^"]+)".*/\1/')
		if [ -z "$WANT" ] || echo "$AFTER" | grep -q "$WANT"; then
			log "back after ~$((i*3))s: $AFTER"
			OK=1
			break
		fi
	fi
done
[ $OK -eq 1 ] || { log "FAILED: device did not come back"; exit 1; }
echo "$INFO" > "upgrade/logs/info-after-$IP.json"
log "=== OK $IP"
