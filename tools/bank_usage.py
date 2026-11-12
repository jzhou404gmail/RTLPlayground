#!/usr/bin/env python3
"""Report the code bank usage of a linked RTLPlayground image.

The linker gives every bank a fixed 48 KiB window; when one is full the build
either fails or silently overwrites the next bank's start. This reads the
Intel HEX the link produced and prints what each bank actually uses.

usage: tools/bank_usage.py output/<MACHINE>/rtlplayground.ihx
"""
import sys

BANKS = {
    "BANK1": 0x14000,
    "BANK2": 0x24000,
    "BANK3": 0x34000,
}
SIZE = 0xC000  # 48 KiB per bank
HOME_START, HOME_SIZE = 0x00000, 0x14000


def read_extents(path):
    top = {name: start for name, start in BANKS.items()}
    home_top = HOME_START
    base = 0
    with open(path) as fh:
        for line in fh:
            line = line.strip()
            if not line.startswith(":"):
                continue
            raw = bytes.fromhex(line[1:])
            count, offset, kind = raw[0], (raw[1] << 8) | raw[2], raw[3]
            if kind == 4:
                base = ((raw[4] << 8) | raw[5]) << 16
            elif kind == 2:
                base = ((raw[4] << 8) | raw[5]) << 4
            elif kind == 0:
                addr = base + offset
                for name, start in BANKS.items():
                    if start <= addr < start + SIZE:
                        top[name] = max(top[name], addr + count)
                if HOME_START <= addr < HOME_START + HOME_SIZE:
                    home_top = max(home_top, addr + count)
    return top, home_top


def main():
    if len(sys.argv) != 2:
        print(__doc__.strip())
        return 2
    top, home_top = read_extents(sys.argv[1])
    rows = [("HOME", home_top - HOME_START, HOME_SIZE)]
    for name, start in BANKS.items():
        rows.append((name, top[name] - start, SIZE))

    over = 0
    print(f"{'bank':6} {'used':>7} {'size':>7} {'free':>7}  {'used%':>6}")
    for name, used, size in rows:
        free = size - used
        if free < 0:
            over += 1
        print(f"{name:6} {used:7} {size:7} {free:7}  {100 * used / size:5.1f}%")
    if over:
        print(f"\n{over} bank(s) over capacity", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
