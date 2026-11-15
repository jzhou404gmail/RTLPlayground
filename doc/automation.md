# Automation

## Authorize

The firmware stores a salted SHA-256 digest, not the password. Fetch the salt
the switch advertises, hash `salt || password` locally, and post only the
digest. `tools/passwd_hash.py --salt <hex>` prints the digest in the same
`passwd_hash <salt> <digest>` form the console uses; the last field is what
`/login` expects.

```bash
SALT=$(curl -s "http://${SWITCH_IP}/auth.json" \
       | python3 -c 'import json,sys; print(json.load(sys.stdin)["salt"])')
DIGEST=$(python3 tools/passwd_hash.py --salt "$SALT" "$PASSWORD" | awk '{print $3}')
curl -c cookies.txt "http://${SWITCH_IP}/login" --data "pwd=${DIGEST}" -i
```

This saves the session cookie in `cookies.txt`. A freshly flashed board has no
password yet: open `http://${SWITCH_IP}/` once and use the first-run page, or
send `passwd_hash <salt> <digest>` on the serial console.

## Upload

Send the firmware via form. `POST /upload` is a state-changing request, so it
needs the `X-RTLP: 1` header:

```bash
curl -b cookies.txt -H 'X-RTLP: 1' \
     "http://${SWITCH_IP}/upload" -F "uploadedfile=@${FIRMWARE_FILE_PATH}" -i
```

You can expect the server to close the connection without responding to the
request. Wait for `SWITCH_IP` to answer again.

## Port status

In the same way as the upload, fetch the JSON pages with the cookie:

```bash
curl -b cookies.txt "http://${SWITCH_IP}/ports.json"
curl -b cookies.txt "http://${SWITCH_IP}/traffic.json"
```

`/status.json` no longer exists: `/ports.json` carries the per-port
configuration and `/traffic.json` the counters (see the API reshape in
`CHANGELOG.md`).
