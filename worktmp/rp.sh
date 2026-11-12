#!/bin/bash
# Run RTLPlayground console commands over HTTP.
# usage: ./rp.sh [host] "cmd1" "cmd2" ...
HOST=${RP_HOST:-192.168.10.247}
SID=$(curl -si -m 10 -d "pwd=${RP_PASS:-1234}" \
	-H "Content-Type: application/x-www-form-urlencoded" \
	"http://$HOST/login" | grep -i '^set-cookie' | sed 's/.*session=\([0-9a-f]*\).*/\1/' | tr -d '\r')
if [ -z "$SID" ]; then
	echo "login failed" >&2
	exit 1
fi
for c in "$@"; do
	echo "== $c"
	curl -s -m 20 -H "Cookie: session=$SID" -X POST --data-binary "$c" "http://$HOST/cmd"
done
