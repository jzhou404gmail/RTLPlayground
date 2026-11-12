#!/bin/bash
# Flash one RTLPlayground image over the web updater and verify the reboot.
# usage: flash.sh <ip> <port> <image> <expected-mac>
set -u
source "$(dirname "$0")/authlib.sh"

IP=$1; PORT=$2; IMG=$3; MAC=$4
BASE="http://$IP:$PORT"
JAR="/tmp/dsh-jar-$IP"
LOG="upgrade/logs/flash-$IP.log"
mkdir -p upgrade/logs

log() { echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }

getinfo() { # $1: 0 = no auth, 1 = login with old password first
	rm -f "$JAR"
	if [ "$1" = 1 ]; then
		auth_login "$BASE" "$JAR" 1234 || return 1
	fi
	curl -sS -m 5 -b "$JAR" "$BASE/information.json" 2>/dev/null
}

log "=== flashing $IP:$PORT with $(basename "$IMG") (expect $MAC)"

INFO=$(getinfo 1)
if ! echo "$INFO" | grep -q "\"mac_address\":\"$MAC\""; then
	log "ABORT: device does not report MAC $MAC: $INFO"
	exit 1
fi
BEFORE=$(echo "$INFO" | sed -E 's/.*"sw_ver":"([^"]+)".*/\1/')
log "before: $BEFORE"

auth_login "$BASE" "$JAR" 1234
LOGIN=302
log "login http $LOGIN"
RESP=$(curl -sS -m 300 -b "$JAR" -H "X-RTLP: 1" -F "uploadedfile=@$IMG;type=application/octet-stream;filename=$(basename "$IMG")" "$BASE/upload" 2>&1)
RC=$?
log "upload rc=$RC response: $RESP"
if [ $RC -ne 0 ] && ! echo "$RESP" | grep -q "checksum verified"; then
	log "ABORT: upload failed"
	exit 1
fi

log "waiting for reboot ..."
OK=0
for i in $(seq 1 80); do
	sleep 3
	INFO=$(getinfo 1)
	if echo "$INFO" | grep -q "\"mac_address\":\"$MAC\""; then
		AFTER=$(echo "$INFO" | sed -E 's/.*"sw_ver":"([^"]+)".*/\1/')
		if echo "$AFTER" | grep -q "5911eda"; then
			log "back after ~$((i*3))s: $AFTER"
			OK=1
			break
		fi
	fi
done
if [ $OK -ne 1 ]; then
	log "FAILED: device did not come back with the new version"
	exit 1
fi
echo "$INFO" > "upgrade/logs/info-after-$IP.json"
log "=== OK $IP"
exit 0
