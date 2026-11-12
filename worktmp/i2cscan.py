#!/usr/bin/env python3
"""Probe the RTL837x I2C controller of a running RTLPlayground switch over the
HTTP console, to find which (SDA,SCL) pin pair reaches the SFP EEPROMs."""
import re
import sys
import http.client

HOST = sys.argv[1] if len(sys.argv) > 1 else "192.168.10.247"
SID = None

PIN_MUX_1 = 0x7F90
I2C_CTRL = 0x0418
I2C_IN = 0x0420
I2C_OUT = 0x0424

# pin -> (bit_of_first_bit_of_2bit_field, value_that_selects_the_function)
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
    c.request("POST", "/cmd", body=cmd, headers={"Cookie": "session=%s" % SID, "Content-Type": "text/plain"})
    r = c.getresponse()
    body = r.read().decode(errors="replace")
    c.close()
    return body


def regget(addr):
    out = run("regget %04x" % addr)
    m = re.search(r"VAL:\s*0x([0-9a-fA-F]+)", out)
    if not m:
        raise RuntimeError("regget failed for %04x: %r" % (addr, out))
    return int(m.group(1), 16)


def regset(addr, val):
    out = run("regset %04x %08x" % (addr, val))
    return out


def mux_for(sda, scl):
    """Return the PIN_MUX_1 value that routes SDA/SCL to the I2C function."""
    val = regget(PIN_MUX_1)
    # SCL3 / SDA3 share bits 5-6 and use 0b10
    for bus, field in ((scl, SCL_FIELD[scl]), (sda, SDA_FIELD[sda])):
        if bus == 3:
            val = (val & ~(0x3 << field)) | (0x2 << field)
        elif bus == 4:
            val = val & ~(0x1 << field)          # SDA4: single bit, 0 = I2C
        else:
            val = (val & ~(0x3 << field)) | (0x1 << field)
    return val


def i2c_read_byte(sda, scl, memaddr=0, dev=0x50):
    """Read one byte from the EEPROM, return None on NAK/error."""
    i2c = (scl << 5) | (sda << 2)
    regset(I2C_IN, memaddr)
    val = (0x10 << 16) | (((dev >> 5) | i2c) << 8) | (((dev << 3) & 0xFF) | 0x1)
    regset(I2C_CTRL, val)
    for _ in range(20):
        st = regget(I2C_CTRL)
        if not (st & 0x1):
            break
    else:
        return "busy"
    if st & 0x2:
        return None                    # NAK: nothing at that address
    return regget(I2C_OUT) & 0xFF


if __name__ == "__main__":
    if not login():
        print("login failed")
        sys.exit(1)
    print("chip id 0004: 0x%08x" % regget(0x0004))
    print("PIN_MUX_1 before: 0x%08x" % regget(PIN_MUX_1))
    print("GPIO in 0044: 0x%08x  0048: 0x%08x" % (regget(0x44), regget(0x48)))

    found = []
    for sda in range(5):
        for scl in range(4):
            mux = mux_for(sda, scl)
            regset(PIN_MUX_1, mux)
            got = i2c_read_byte(sda, scl, 0x00)
            extra = ""
            if isinstance(got, int):
                rate = i2c_read_byte(sda, scl, 0x0C)
                extra = " rate=%s" % (("0x%02x" % rate) if isinstance(rate, int) else rate)
                found.append((sda, scl))
            print("SDA%d SCL%d mux=0x%08x read=%s%s" %
                  (sda, scl, mux, got if not isinstance(got, int) else "0x%02x" % got, extra))
    print("buses answering:", found)
