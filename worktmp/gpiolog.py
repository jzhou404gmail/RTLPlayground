#!/usr/bin/env python3
"""Log the RTL837x GPIO input registers once a second, to catch a switch."""
import re
import sys
import time
import http.client

HOST = sys.argv[1] if len(sys.argv) > 1 else "192.168.10.247"
SECONDS = int(sys.argv[2]) if len(sys.argv) > 2 else 45
SID = None


def conn():
    return http.client.HTTPConnection(HOST, 80, timeout=10)


def login():
    global SID
    c = conn()
    c.request("POST", "/login", body="pwd=1234",
              headers={"Content-Type": "application/x-www-form-urlencoded"})
    r = c.getresponse()
    sc = r.getheader("Set-Cookie") or ""
    r.read()
    c.close()
    m = re.search(r"session=([0-9a-f]+)", sc)
    SID = m.group(1) if m else None


def run(cmd):
    c = conn()
    c.request("POST", "/cmd", body=cmd,
              headers={"Cookie": "session=%s" % SID, "Content-Type": "text/plain"})
    r = c.getresponse()
    body = r.read().decode(errors="replace")
    c.close()
    return body


def regget(addr):
    out = run("regget %04x" % addr)
    m = re.search(r"VAL:\s*0x([0-9a-fA-F]+)", out)
    return int(m.group(1), 16) if m else None


for attempt in range(6):
    login()
    if SID:
        break
    time.sleep(2)
if not SID:
    sys.exit("login failed")
t0 = time.time()
last = None
while time.time() - t0 < SECONDS:
    lo = regget(0x44)
    hi = regget(0x48)
    if lo is None or hi is None:
        login()
        lo = regget(0x44)
        hi = regget(0x48)
    if lo is None or hi is None:
        print("%6.1f  read failed" % (time.time() - t0))
        time.sleep(1)
        continue
    cur = (lo, hi)
    mark = ""
    if last is not None and cur != last:
        diff_lo = last[0] ^ lo
        diff_hi = last[1] ^ hi
        bits = [str(i) for i in range(32) if (diff_lo >> i) & 1]
        bits += [str(32 + i) for i in range(32) if (diff_hi >> i) & 1]
        mark = "  CHANGED gpio " + ",".join(bits)
    print("%6.1f  0x%08x 0x%08x%s" % (time.time() - t0, lo, hi, mark), flush=True)
    last = cur
    time.sleep(0.8)
