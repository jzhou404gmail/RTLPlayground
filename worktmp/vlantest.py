#!/usr/bin/env python3
"""Exercise the VLAN settings against the panel VLAN switch, over HTTP."""
import json
import re
import sys
import time
import http.client

HOST = sys.argv[1] if len(sys.argv) > 1 else "192.168.10.247"
SID = None


def conn():
    return http.client.HTTPConnection(HOST, 80, timeout=20)


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
    return SID


def cmd(c):
    c_ = conn()
    c_.request("POST", "/cmd", body=c,
               headers={"Cookie": "session=%s" % SID, "Content-Type": "text/plain"})
    r = c_.getresponse()
    body = r.read().decode(errors="replace")
    c_.close()
    return body.strip()


def get(path):
    c_ = conn()
    c_.request("GET", path, headers={"Cookie": "session=%s" % SID})
    r = c_.getresponse()
    body = r.read().decode(errors="replace")
    c_.close()
    return body


def reg(addr):
    out = cmd("regget %04x" % addr)
    m = re.search(r"VAL:\s*0x([0-9a-fA-F]+)", out)
    return int(m.group(1), 16) if m else None


def masks():
    return [reg(a) for a in (0x50D0, 0x50D4, 0x50D8, 0x50DC)]


def vlan_json(vid):
    try:
        return json.loads(get("/vlan.json?vid=%d" % vid))
    except Exception:
        return None


def info():
    return json.loads(get("/information.json"))


def say(step, detail=""):
    print("\n### %s" % step)
    if detail:
        print(detail)


def show_state(tag):
    m = masks()
    i = info()
    print("  %-28s switch=%-3s function=%-3s masks=%s pvid1=%s" % (
        tag, i.get("vlan_switch"), i.get("vlan_switch_en"),
        " ".join("%03x" % x for x in m),
        (vlan_json(1) or {}).get("pvid")))


if not login():
    sys.exit("login failed")
time.sleep(1)

say("Baseline")
show_state("start")
print("  vlan 1 json:", vlan_json(1))

say("A. function ON + panel switch ON: create VLAN 20 on LAN2/LAN3")
print(" ", cmd("vlan 20 2 3").replace("\n", " | "))
print(" ", cmd("pvid 2 20").replace("\n", " | "))
print(" ", cmd("pvid 3 20").replace("\n", " | "))
time.sleep(1.2)
print("  vlan 20 json:", vlan_json(20))
print("  vlan 1  pvid mask:", (vlan_json(1) or {}).get("pvid"))
show_state("after VLAN 20")

say("B. turn the panel-switch function OFF")
print(" ", cmd("vlan switch off").replace("\n", " | "))
time.sleep(1.5)
show_state("function off")
print("  vlan 20 still there:", vlan_json(20))
print(" ", cmd("vlan 21 2 3").replace("\n", " | "))
time.sleep(0.5)
print("  vlan 21 json:", vlan_json(21))

say("C. function ON again (panel switch ON): VLAN page must still work")
print(" ", cmd("vlan switch on").replace("\n", " | "))
time.sleep(1.5)
show_state("function on")
print(" ", cmd("vlan 22 2 3").replace("\n", " | "))
time.sleep(0.5)
print("  vlan 22 json:", vlan_json(22))
print("  isolation.json:", get("/isolation.json"))

say("D. cleanup")
print(" ", cmd("pvid 2 1").replace("\n", " | "))
print(" ", cmd("pvid 3 1").replace("\n", " | "))
for v in (20, 21, 22):
    print(" ", cmd("vlan %d d" % v).strip() or "(deleted %d)" % v)
time.sleep(1.5)
show_state("restored")
print("  vlan 1 pvid mask:", (vlan_json(1) or {}).get("pvid"))
print("  vlan show:", cmd("vlan show").replace("\n", " | "))
