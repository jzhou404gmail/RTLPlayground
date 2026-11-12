#!/usr/bin/env python3
"""Read SFP EEPROM pages over the HTTP console (I2C controller poke)."""
import re
import sys
import http.client

HOST = sys.argv[1] if len(sys.argv) > 1 else "192.168.10.247"
SID = None
PIN_MUX_1 = 0x7F90
I2C_CTRL, I2C_IN, I2C_OUT = 0x0418, 0x0420, 0x0424
SCL_FIELD = {0: 7, 1: 11, 2: 15, 3: 5}
SDA_FIELD = {0: 9, 1: 13, 2: 17, 3: 5, 4: 29}


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


def regset(addr, val):
    run("regset %04x %08x" % (addr, val))


def mux_for(sda, scl):
    val = regget(PIN_MUX_1)
    for bus, field in ((scl, SCL_FIELD[scl]), (sda, SDA_FIELD[sda])):
        if bus == 3:
            val = (val & ~(0x3 << field)) | (0x2 << field)
        elif bus == 4:
            val = val & ~(0x1 << field)
        else:
            val = (val & ~(0x3 << field)) | (0x1 << field)
    return val


def i2c_block(sda, scl, memaddr, length, dev=0x50):
    """Read `length` (1..16) bytes starting at EEPROM byte memaddr."""
    i2c = (scl << 5) | (sda << 2)
    regset(I2C_IN, memaddr)
    val = ((0x10 | (length - 1)) << 16) | (((dev >> 5) | i2c) << 8) | \
          (((dev << 3) & 0xFF) | 0x1)
    regset(I2C_CTRL, val)
    for _ in range(30):
        st = regget(I2C_CTRL)
        if not (st & 0x1):
            break
    else:
        return None
    if st & 0x2:
        return None
    out = []
    for i in range(length):
        if i % 4 == 0:
            for _try in range(3):
                try:
                    v = regget(I2C_OUT + i)
                    break
                except RuntimeError:
                    continue
            else:
                return None
        b = [v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >> 24) & 0xff][i % 4]
        out.append(b)
    return bytes(out)


def ascii_field(b):
    return b.decode("ascii", "replace").strip()


if __name__ == "__main__":
    if not login():
        sys.exit("login failed")
    for sda, scl in ((4, 3), (3, 3)):
        regset(PIN_MUX_1, mux_for(sda, scl))
        head = i2c_block(sda, scl, 0, 16)
        if head is None:
            print("SDA%d/SCL%d: no answer" % (sda, scl))
            continue
        ident = head[0]
        vendor = i2c_block(sda, scl, 20, 16)
        model = i2c_block(sda, scl, 40, 16)
        serial = i2c_block(sda, scl, 68, 16)
        rate = i2c_block(sda, scl, 12, 1)
        print("SDA%d/SCL%d: ident=0x%02x %s|%s|%s rate=0x%02x" % (
            sda, scl, ident,
            ascii_field(vendor) if vendor else "?",
            ascii_field(model) if model else "?",
            ascii_field(serial) if serial else "?",
            rate[0] if rate else 0))
