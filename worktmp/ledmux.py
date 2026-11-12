#!/usr/bin/env python3
"""Set the RTL837x LED mux over the HTTP console and read the GPIO inputs."""
import re
import sys
import http.client

HOST = sys.argv[1] if len(sys.argv) > 1 else "192.168.10.247"
SID = None
MUX0 = 0x65E0


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
    if not m:
        raise RuntimeError("regget %04x: %r" % (addr, out))
    return int(m.group(1), 16)


def set_mux(pads):
    """pads: list of 28 6-bit signal ids."""
    assert len(pads) == 28
    for reg in range(6):
        fields = pads[reg * 5: reg * 5 + 5]
        if reg == 5:
            while len(fields) < 5:
                fields.append(0)
            val = sum((v & 0x3F) << (6 * i) for i, v in enumerate(fields[:5]))
            val &= 0x0000FFFF  # MUX_6 only has 3 fields + 1 spare
        else:
            val = sum((v & 0x3F) << (6 * i) for i, v in enumerate(fields))
        run("regset %04x %08x" % (MUX0 + reg * 4, val))


def read_mux():
    out = []
    for reg in range(6):
        v = regget(MUX0 + reg * 4)
        out.extend([(v >> (6 * i)) & 0x3F for i in range(5) if reg * 5 + i < 28])
    return out[:28]


def gpio():
    return regget(0x44), regget(0x48)


def bits(v, n):
    return "".join("1" if (v >> i) & 1 else "0" for i in range(n))


if __name__ == "__main__":
    if not login():
        sys.exit("login failed")
    cmd = sys.argv[2]
    if cmd == "get":
        m = read_mux()
        print("mux:", " ".join("%02x" % x for x in m))
        lo, hi = gpio()
        print("gpio 0-27: %s" % bits(lo, 28))
        print("gpio 0x44=0x%08x 0x48=0x%08x" % (lo, hi))
    elif cmd == "float":
        set_mux([0x3F] * 28)
        lo, hi = gpio()
        print("all pads 0x3f, gpio 0-27: %s" % bits(lo, 28))
        print("gpio 0x44=0x%08x 0x48=0x%08x" % (lo, hi))
    elif cmd == "set":
        pads = [int(x, 16) for x in sys.argv[3:31]]
        set_mux(pads)
        print("mux:", " ".join("%02x" % x for x in read_mux()))
        lo, hi = gpio()
        print("gpio 0-27: %s" % bits(lo, 28))
    elif cmd == "test":
        # drive one group at a time with a chosen signal
        sig = int(sys.argv[3], 16)
        groups = [(0, 7), (7, 14), (14, 21), (21, 28)]
        for a, b in groups:
            pads = [0x02] * 28
            for i in range(a, b):
                pads[i] = sig
            set_mux(pads)
            print("pads %d-%d -> 0x%02x, gpio 0-27: %s" % (a, b - 1, sig, bits(gpio()[0], 28)))
