#!/usr/bin/env python3
"""
Capture a switch's serial console, with timestamps, without needing pyserial.

The RTL837x console needs 8N1 raw mode; line settings are applied with stty
before the port is opened, so nothing but the Python standard library is
required.

Examples
--------
# capture the boot log of the stock firmware (9600 baud) for 4 minutes
python3 console_capture.py -p /dev/ttyUSB0 -b 9600 -t 240 -o output/console_boot.txt

# interactive-ish: capture RTLPlayground's console (115200 baud) for 5 minutes
python3 console_capture.py -b 115200 -t 300

# short read, to check whether something is printing
python3 console_capture.py -t 5
"""

import argparse
import os
import select
import subprocess
import sys
import time


def configure(port: str, baud: int) -> None:
    """Put the tty into raw 8N1 mode at the requested baud rate."""
    cmd = [
        "stty", "-F", port, str(baud), "cs8", "-cstopb", "-parenb",
        "raw", "-echo", "-echoe", "-echok", "-crtscts", "-ixon", "-ixoff",
    ]
    subprocess.run(cmd, check=True)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--port", default="/dev/ttyUSB0", help="serial device")
    ap.add_argument("-b", "--baud", type=int, default=9600, help="baud rate")
    ap.add_argument("-t", "--seconds", type=float, default=60.0, help="capture duration")
    ap.add_argument("-o", "--out", default=None,
                    help="also write a raw (unmodified) byte log to this file")
    ap.add_argument("-n", "--note", default="", help="text to print as a header")
    ap.add_argument("-s", "--send", action="append", default=[],
                    help="escape sequence to send before capturing, e.g. -s '\\r' or -s '?'")
    args = ap.parse_args()

    try:
        configure(args.port, args.baud)
    except Exception as exc:                       # noqa: BLE001
        print(f"cannot configure {args.port} @ {args.baud}: {exc}", file=sys.stderr)
        return 2

    try:
        # Read-write on purpose: with a read-only fd some USB-TTL adapters leave
        # the switch's RX input floating, which floods the console with
        # framing-error junk. Holding the port open for writing keeps the
        # adapter's TX at idle level. Nothing is sent unless --send is used.
        fd = os.open(args.port, os.O_RDWR | os.O_NONBLOCK)
    except OSError as exc:
        print(f"cannot open {args.port}: {exc}", file=sys.stderr)
        return 2

    for chunk in args.send:
        payload = chunk.encode().decode("unicode_escape").encode("latin-1")
        os.write(fd, payload)
        print(f"# sent {payload!r}")
        sys.stdout.flush()
        time.sleep(0.4)

    raw = open(args.out, "wb") if args.out else None
    header = f"# {args.port} @ {args.baud} 8N1, {args.seconds:g}s, started {time.strftime('%Y-%m-%d %H:%M:%S')}"
    if args.note:
        header += f"  ({args.note})"
    print(header)
    sys.stdout.flush()

    t0 = time.time()
    total = 0
    try:
        while time.time() - t0 < args.seconds:
            readable, _, _ = select.select([fd], [], [], 0.2)
            if not readable:
                continue
            try:
                data = os.read(fd, 4096)
            except BlockingIOError:
                continue
            if not data:
                continue
            total += len(data)
            if raw:
                raw.write(data)
                raw.flush()
            stamp = f"[{time.time() - t0:8.3f}] "
            sys.stdout.write(stamp + data.decode("utf-8", "replace").replace("\r\n", "\n"))
            sys.stdout.flush()
    except KeyboardInterrupt:
        pass
    finally:
        os.close(fd)
        if raw:
            raw.close()

    print(f"# captured {total} bytes in {time.time() - t0:.1f}s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
