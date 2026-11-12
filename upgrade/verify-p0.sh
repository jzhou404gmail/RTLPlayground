#!/bin/bash
# Post-flash checks for the P0 sync: sessions, logout, CSRF/headers, /reset.
# usage: verify-p0.sh <ip> <port> <password>
set -u
source "$(dirname "$0")/authlib.sh"
IP=$1; PORT=$2; PW=$3
B="http://$IP:$PORT"
J1=/tmp/v1-$IP; J2=/tmp/v2-$IP
PASS=0; FAIL=0
ok()   { echo "  PASS  $*"; PASS=$((PASS+1)); }
bad()  { echo "  FAIL  $*"; FAIL=$((FAIL+1)); }
check(){ if eval "$2"; then ok "$1"; else bad "$1"; fi; }
code() { curl -s -o /dev/null -w "%{http_code}" "$@"; }
login(){ rm -f "$2"; auth_login "$B" "$2" "$1"; }

echo "== $IP:$PORT =="

AUTH=$(curl -sS -m 5 "$B/auth.json")
echo "  auth.json: $AUTH"
echo "$AUTH" | grep -q '"set":1' && ok "digest is provisioned" || bad "no digest"

login "$PW" "$J1"
check "login A works"            '[ "$(code -b "$J1" "$B/information.json")" = 200 ]'
INFO=$(curl -s -b "$J1" "$B/information.json")
echo "  version: $(echo "$INFO" | grep -o '"sw_ver":"[^"]*"')"
echo "$INFO" | grep -q '"sw_ver":"v0.1.7-' && ok "information.json reports a version" || bad "no version in information.json"

login "$PW" "$J2"
check "login B works (2nd browser)" '[ "$(code -b "$J2" "$B/information.json")" = 200 ]'
check "both sessions are valid"  '[ "$(code -b "$J1" "$B/information.json")" = 200 ] && [ "$(code -b "$J2" "$B/vlan.json")" = 200 ]'

check "POST /cmd without X-RTLP is 403" '[ "$(code -b "$J1" --data "version" "$B/cmd")" = 403 ]'
check "POST /cmd with X-RTLP is 200"    '[ "$(code -b "$J1" -H "X-RTLP: 1" --data "version" "$B/cmd")" = 200 ]'
check "wrong Host is 421"               '[ "$(code -H "Host: evil.example" "$B/information.json")" = 421 ]'

check "logout B answers 302"     '[ "$(code -b "$J2" -X POST -H "X-RTLP: 1" -H "Content-Type: application/x-www-form-urlencoded" "$B/logout")" = 302 ]'
check "B cookie is dead"         '[ "$(code -b "$J2" "$B/vlan.json")" = 401 ]'
check "A session survives B logout" '[ "$(code -b "$J1" "$B/information.json")" = 200 ]'

echo
echo "  /reset test: the switch answers 200 and reboots"
RC=$(code -b "$J1" "$B/reset")
echo "  reset http=$RC"
check "reset answers 200 first" '[ "$RC" = 200 ]'
sleep 12
check "A is logged out after reboot" '[ "$(code -b "$J1" "$B/information.json")" = 401 ]' || true
login "$PW" "$J1"
check "login works again after reboot" '[ "$(code -b "$J1" "$B/information.json")" = 200 ]'

echo
echo "RESULT: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
