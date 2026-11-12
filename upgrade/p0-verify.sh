#!/bin/bash
# Post-deployment acceptance for the four switches: every unauthenticated
# endpoint must be 401, the password must work, and the startup configuration
# and VLAN state must be unchanged apart from the added "passwd" line.
set -u
cd "$(dirname "$0")/.." || exit 1

OUT=upgrade/logs/v014-verify-$(date +%Y%m%d-%H%M%S).txt
: > "$OUT"
say() { echo "$*" | tee -a "$OUT"; }

for ip in 201 202 203 204; do
	B="http://192.168.32.$ip:8000"
	JAR="upgrade/logs/.jar-$ip"
	say "================ $B ================"

	# 1. unauthenticated surface
	info=$(curl -s -o /dev/null -m 5 -w '%{http_code}' "$B/information.json")
	cfg=$(curl -s -o /dev/null -m 5 -w '%{http_code}' "$B/config")
	cmd=$(curl -s -o /dev/null -m 5 -w '%{http_code}' -X POST "$B/cmd" --data version)
	up=$(curl -s -o /dev/null -m 5 -w '%{http_code}' -F uploadedfile=@/etc/hostname "$B/upload")
	rst=$(curl -s -o /dev/null -m 5 -w '%{http_code}' "$B/reset")
	clr=$(curl -s -o /dev/null -m 5 -w '%{http_code}' "$B/cmd_log_clear")
	say "unauth: info=$info config=$cfg cmd=$cmd upload=$up reset=$rst log_clear=$clr"

	# 2. wrong and empty passwords must not hand out a cookie
	wrong=$(curl -s -D- -o /dev/null -m 5 -X POST "$B/login" --data 'pwd=definitely-wrong' | grep -ci '^set-cookie')
	empty=$(curl -s -D- -o /dev/null -m 5 -X POST "$B/login" --data 'pwd=' | grep -ci '^set-cookie')
	say "login: wrong -> cookies=$wrong, empty -> cookies=$empty"

	# 3. correct password
	rm -f "$JAR"
	SALT=$(curl -sS -m 5 "$B/auth.json" 2>/dev/null | python3 -c '
import json,sys
try:
    d=json.load(sys.stdin)
    print(d.get("salt","") if d.get("set") else "")
except Exception:
    print("")
' 2>/dev/null)
	if [ -n "$SALT" ]; then
		DIG=$(python3 -c 'import hashlib,sys
print(hashlib.sha256(bytes.fromhex(sys.argv[1])+sys.argv[2].encode()).hexdigest())' "$SALT" 1234)
	else
		DIG=1234
	fi
	LOGIN=$(curl -s -D- -o /dev/null -m 5 -c "$JAR" -X POST "$B/login" --data "pwd=$DIG")
	code=$(echo "$LOGIN" | head -1)
	ck=$(echo "$LOGIN" | grep -i '^set-cookie' | tr -d '\r')
	say "login 1234: $code"
	say "  $ck"
	case "$ck" in
	*session=*HttpOnly*SameSite=Strict*) : ;;
	*) say "  WARN: cookie attributes missing" ;;
	esac
	len=$(echo "$ck" | sed -nE 's/.*session=([0-9a-f]+).*/\1/p' | awk '{print length}')
	say "  session id hex length: ${len:-0} (expect 32)"

	# 4. authenticated views
	curl -s -m 5 -b "$JAR" "$B/config" -o "upgrade/logs/p0-config-live-$ip.txt"
	curl -s -m 5 -b "$JAR" "$B/vlanlist" -o "upgrade/logs/p0-vlanlist-live-$ip.json"
	curl -s -m 5 -b "$JAR" "$B/information.json" -o "upgrade/logs/p0-info-live-$ip.json"
	INFO=$(python3 - "upgrade/logs/p0-info-live-$ip.json" <<'PY'
import json,sys
d=json.load(open(sys.argv[1]))
print(f'{d["sw_ver"]} | {d["model"]} | {d["hostname"]} | {d["ip_address"]} | port {d["http_port"]}')
PY
)
	say "info: $INFO"

	# 5. config unchanged except the passwd line
	python3 - "$ip" <<'PY' 2>&1 | tee -a "$OUT"
import sys
ip=sys.argv[1]
def L(p): return [l.strip() for l in open(p, encoding='utf-8', errors='replace') if l.strip()]
seeded=L(f'upgrade/logs/p0-config-seeded-192.168.32.{ip}.txt')
live=L(f'upgrade/logs/p0-config-live-{ip}.txt')
base=L(f'upgrade/logs/p0-config-before-192.168.32.{ip}.txt')
print('  config: baseline', len(base), 'lines; seeded', len(seeded), '; live', len(live))
if live != seeded:
    print('  config DIFF vs seeded:'); 
    import difflib
    print(''.join(difflib.unified_diff(seeded, live, 'seeded', 'live', lineterm='')))
else:
    print('  config: identical to seeded baseline (only passwd added)')
PY

	# 6. VLAN state unchanged
	if diff -q "upgrade/logs/p0-vlanlist-before-$ip.json" "upgrade/logs/p0-vlanlist-live-$ip.json" >/dev/null; then
		say "vlanlist: unchanged ($(cat "upgrade/logs/p0-vlanlist-live-$ip.json"))"
	else
		say "vlanlist: CHANGED -> $(cat "upgrade/logs/p0-vlanlist-live-$ip.json")"
	fi

	# 7. links still up
	say "links: $(curl -s -m 5 -b "$JAR" -H "X-RTLP: 1" -X POST "$B/cmd" --data stat | awk -F'\t' '$1 ~ /^[0-9]+$/ {printf "p%s=%s ", $1, $3}')"
	say "time:  $(curl -s -m 5 -b "$JAR" -H "X-RTLP: 1" -X POST "$B/cmd" --data time | tr '\n' ' ')"
	rm -f "$JAR"
	say ""
done

say "gateway ping: $(ping -c 2 -W 2 192.168.32.254 2>&1 | tail -2 | tr '\n' ' ')"
say "saved: $OUT"
