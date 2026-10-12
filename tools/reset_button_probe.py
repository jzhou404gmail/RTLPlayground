#!/usr/bin/env python3
"""Watch the reset button of a running RTLPlayground switch over HTTP.

Why a script: a short press is only visible as a GPIO level that goes low for a
few seconds and as the reboot that follows, and the board being watched may have
no serial console. Once per interval this logs

    btn     the reset pin level (GPIO54 by default: bit 22 of the GPIO bank-1
            input register) - 0 means pressed
    up      seconds since boot, from the console's tick counter
    host    the hostname from /information.json

which is enough to tell the three outcomes of handle_button() apart:

    btn low < 3 s, nothing else             "short press, no action"
    btn low > 3 s, reboot, same hostname,   reset_chip() - the board restarts with
      "up" back near 0                        its startup configuration untouched
    btn low > 10 s, reboot, hostname back   flash_default_config() + reset_chip()
      to the one built into the image

Put a marker in the startup configuration first (System tab, or a POST to
/config) - for example a "hostname BUTTON-TEST" line - and reboot, so that the
factory-reset case is unmistakable in the log.

Usage: reset_button_probe.py <ip> [seconds] [password] [pin]
"""
import http.cookiejar
import json
import re
import sys
import time
import urllib.request

IP = sys.argv[1] if len(sys.argv) > 1 else "192.168.10.247"
DUR = float(sys.argv[2]) if len(sys.argv) > 2 else 600.0
PWD = sys.argv[3] if len(sys.argv) > 3 else "1234"
PIN = int(sys.argv[4]) if len(sys.argv) > 4 else 54
INTERVAL = 2.0
LOG = "reset_button_probe-%s.log" % IP.replace(".", "_")
BANK, BIT = PIN // 32, PIN % 32
UPTIME_TICKS_PER_S = 200          # SYS_TICK_HZ, the firmware's tick rate

cj = http.cookiejar.CookieJar()
op = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cj))


def login():
    req = urllib.request.Request(
        "http://%s/login" % IP, data=("pwd=%s" % PWD).encode(),
        headers={"Content-Type": "application/x-www-form-urlencoded"})
    try:
        op.open(req, timeout=5).read()
        return True
    except Exception:
        return False


def cmd(body):
    req = urllib.request.Request("http://%s/cmd" % IP, data=body.encode())
    with op.open(req, timeout=10) as r:
        return r.read().decode("utf-8", "replace")


def hostname():
    with op.open("http://%s/information.json" % IP, timeout=5) as r:
        return json.loads(r.read().decode("utf-8", "replace"))["hostname"]


def sample():
    out = cmd("gpio\ntime\n")
    m = re.search(r"GPIO %d:\s*([0-9a-f]+)" % BANK, out)
    btn = "?" if not m else str((int(m.group(1), 16) >> BIT) & 1)
    t = re.search(r"Tick counter: 0x([0-9a-f]+)", out)
    up = int(t.group(1), 16) // UPTIME_TICKS_PER_S if t else -1
    return "btn=%s up=%-6d host=%s" % (btn, up, hostname())


def main():
    print("watching GPIO%d of %s for %.0f s, logging to %s" % (PIN, IP, DUR, LOG))
    presses, reboots, prev_up, prev_btn = [], [], None, None
    end = time.time() + DUR
    with open(LOG, "w") as f:
        f.write("# reset button (GPIO%d) probe of %s; btn=0 means pressed\n" % (PIN, IP))
        login()
        while time.time() < end:
            ts = time.strftime("%H:%M:%S")
            try:
                line = sample()
                m = re.search(r"btn=(\S+)\s+up=(-?\d+)\s+host=(\S+)", line)
                if not m:
                    raise ValueError("unparsed: " + line)
                btn, up, host = m.group(1), int(m.group(2)), m.group(3)
                if btn == "0" and prev_btn != "0":
                    presses.append(ts)
                if prev_up is not None and up < prev_up:      # uptime went backwards
                    reboots.append((ts, host))
                prev_btn, prev_up = btn, up
            except Exception as e:
                line = "UNREACHABLE (%s)" % type(e).__name__
                login()
            out = "%s %s" % (ts, line)
            print(out, flush=True)
            f.write(out + "\n")
            f.flush()
            time.sleep(INTERVAL)
    print("\npresses seen: %s" % (", ".join(presses) or "none"))
    for ts, host in reboots:
        print("reboot at %s -> hostname %s" % (ts, host))


main()
