#!/bin/bash
# Guarded firmware upgrade: verify MAC, model and flash JEDEC ID before
# uploading, then wait for the expected version to come back.
# usage: flash_checked.sh <ip> <port> <image> <mac> <model> <jedec> [version]
set -u
source "$(dirname "$0")/authlib.sh"

IP=$1; PORT=$2; IMG=$3; MAC=$4; MODEL=$5; JEDEC=$6; WANT_VER=${7:-v0.1.1}
BASE="http://$IP:$PORT"
JAR="/tmp/dsh-jar-$IP"
LOG="upgrade/logs/v011-flash-$IP.log"
mkdir -p upgrade/logs

log() { echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }

getinfo() {
	local out
	out=$(curl -sS -m 5 "$BASE/information.json" 2>/dev/null || true)
	case "$out" in
	*'"mac_address"'*) echo "$out"; return 0 ;;
	esac
	rm -f "$JAR"
	auth_login "$BASE" "$JAR" 1234 || true
	curl -sS -m 5 -b "$JAR" "$BASE/information.json" 2>/dev/null
}

log "=== $IP:$PORT -> $(basename "$IMG") ($MODEL, flash $JEDEC)"
INFO=$(getinfo)
if [ -z "$INFO" ]; then log "ABORT: no information.json"; exit 1; fi
echo "$INFO" > "/tmp/info-pre-$IP.json"

python3 - "$MAC" "$MODEL" "$IP" <<'PY' || exit 1
import json, sys
mac, model, ip = sys.argv[1], sys.argv[2], sys.argv[3]
d = json.load(open(f'/tmp/info-pre-{ip}.json'))
print(f"  pre: ip={d['ip_address']} mac={d['mac_address']} model={d['model']!r} hw={d['hw_ver']!r} ver={d['sw_ver']} flash={d['flash_size']} port={d['http_port']}")
if d['mac_address'].lower() != mac.lower():
    print(f"ABORT: MAC {d['mac_address']} != expected {mac}"); sys.exit(1)
if d['model'] != model:
    print(f"ABORT: model {d['model']!r} != expected {model!r}"); sys.exit(1)
PY

J=$(curl -sS -m 8 -b "$JAR" -H "X-RTLP: 1" -d "flash j" "$BASE/cmd" 2>/dev/null)
MID=$(echo "$J" | sed -nE 's/.*Manufacturer ID:[[:space:]]*0x([0-9a-fA-F]+).*/\1/p' | head -1)
MT=$(echo "$J" | sed -nE 's/.*Memory Type:[[:space:]]*0x([0-9a-fA-F]+).*/\1/p' | head -1)
CAP=$(echo "$J" | sed -nE 's/.*Capacity:[[:space:]]*0x([0-9a-fA-F]+).*/\1/p' | head -1)
GOT=$(printf '%s%s%s' "$MID" "$MT" "$CAP" | tr 'A-F' 'a-f')
log "  flash id: ${GOT:-unreadable} (expected $JEDEC)"
if [ "$GOT" != "$JEDEC" ]; then
	log "ABORT: flash JEDEC mismatch, refusing to flash"
	exit 1
fi

RESP=$(curl -sS -m 300 -b "$JAR" -H "X-RTLP: 1" -F "uploadedfile=@$IMG;type=application/octet-stream;filename=$(basename "$IMG")" "$BASE/upload" 2>&1)
RC=$?
log "  upload rc=$RC: $RESP"
case "$RESP" in
*"checksum verified"*) ;;
*) log "ABORT: upload was not verified"; exit 1 ;;
esac

log "  waiting for reboot ..."
OK=0
for i in $(seq 1 100); do
	sleep 3
	INFO=$(getinfo)
	if [ -n "$INFO" ] && echo "$INFO" | grep -q "\"mac_address\":\"$MAC\""; then
		VER=$(echo "$INFO" | sed -nE 's/.*"sw_ver":"([^"]+)".*/\1/p')
		case "$VER" in
		*"$WANT_VER"*)
			log "  back after ~$((i*3))s: $VER"
			OK=1
			break
			;;
		esac
	fi
done
if [ $OK -ne 1 ]; then
	log "FAILED: device did not come back with $WANT_VER"
	exit 1
fi
echo "$INFO" > "upgrade/logs/v011-info-after-$IP.json"
log "=== OK $IP"
exit 0
