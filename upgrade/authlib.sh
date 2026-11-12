#!/bin/bash
# Digest login helper. The switch never receives the password: /auth.json gives
# the salt, this function computes SHA-256(salt||password) and posts the digest.
# If the switch has no digest yet (pre-migration), it falls back to plaintext.
auth_login() {
	local base=$1 jar=$2 pw=$3 salt digest
	salt=$(curl -sS -m 5 "$base/auth.json" 2>/dev/null | python3 -c '
import json,sys
try:
    d=json.load(sys.stdin)
    print(d.get("salt","") if d.get("set") else "")
except Exception:
    print("")
' 2>/dev/null)
	if [ -n "$salt" ]; then
		digest=$(python3 -c 'import hashlib,sys
print(hashlib.sha256(bytes.fromhex(sys.argv[1])+sys.argv[2].encode()).hexdigest())' "$salt" "$pw")
		curl -sS -m 5 -o /dev/null -c "$jar" -d "pwd=$digest" "$base/login" 2>/dev/null
	else
		curl -sS -m 5 -o /dev/null -c "$jar" -d "pwd=$pw" "$base/login" 2>/dev/null
	fi
}
