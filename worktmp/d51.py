#!/usr/bin/env python3
"""Minimal 8051 disassembler, enough to read the Realtek stock firmware."""
import sys

COLS = {
    0x0: {0x00: ("NOP", []), 0x10: ("JBC", ["bit", "rel"]), 0x20: ("JB", ["bit", "rel"]),
          0x30: ("JNB", ["bit", "rel"]), 0x40: ("JC", ["rel"]), 0x50: ("JNC", ["rel"]),
          0x60: ("JZ", ["rel"]), 0x70: ("JNZ", ["rel"]), 0x80: ("SJMP", ["rel"]),
          0x90: ("MOV DPTR,#", ["a16"]), 0xA0: ("ORL C,/", ["bit"]),
          0xB0: ("ANL C,/", ["bit"]), 0xC0: ("PUSH", ["dir"]), 0xD0: ("POP", ["dir"]),
          0xE0: ("MOVX A,@DPTR", []), 0xF0: ("MOVX @DPTR,A", [])},
    0x1: {0x00: ("AJMP", []), 0x10: ("ACALL", [])},
    0x2: {0x00: ("LJMP", ["a16"]), 0x10: ("LCALL", ["a16"]), 0x20: ("RET", []),
          0x30: ("RETI", []), 0x40: ("ORL", ["dir", "A"]), 0x50: ("ANL", ["dir", "A"]),
          0x60: ("XRL", ["dir", "A"]), 0x70: ("ORL C,", ["bit"]),
          0x80: ("ANL C,", ["bit"]), 0x90: ("MOV", ["bit", "C"]),
          0xA0: ("ANL C,/", ["bit"]), 0xB0: ("CPL", ["bit"]),
          0xC0: ("CLR", ["bit"]), 0xD0: ("SETB", ["bit"]),
          0xE0: ("MOVX A,@R0", []), 0xF0: ("MOVX @R0,A", [])},
    0x3: {0x00: ("RR A", []), 0x10: ("RRC A", []), 0x20: ("RL A", []),
          0x30: ("RLC A", []), 0x40: ("ORL", ["dir", "imm"]), 0x50: ("ANL", ["dir", "imm"]),
          0x60: ("XRL", ["dir", "imm"]), 0x70: ("JMP @A+DPTR", []),
          0x80: ("MOVC A,@A+PC", []), 0x90: ("MOVC A,@A+DPTR", []),
          0xA0: ("INC DPTR", []), 0xB0: ("CPL C", []), 0xC0: ("CLR C", []),
          0xD0: ("SETB C", []), 0xE0: ("MOVX A,@R1", []), 0xF0: ("MOVX @R1,A", [])},
    0x4: {0x00: ("INC A", []), 0x10: ("DEC A", []), 0x20: ("ADD A,#", ["imm"]),
          0x30: ("ADDC A,#", ["imm"]), 0x40: ("ORL A,#", ["imm"]),
          0x50: ("ANL A,#", ["imm"]), 0x60: ("XRL A,#", ["imm"]),
          0x70: ("MOV A,#", ["imm"]), 0x80: ("DIV AB", []), 0x90: ("SUBB A,#", ["imm"]),
          0xA0: ("MUL AB", []), 0xB0: ("CJNE A,#", ["imm", "rel"]),
          0xC0: ("SWAP A", []), 0xD0: ("DA A", []), 0xE0: ("CLR A", []),
          0xF0: ("CPL A", [])},
    0x5: {0x00: ("INC", ["dir"]), 0x10: ("DEC", ["dir"]), 0x20: ("ADD A,", ["dir"]),
          0x30: ("ADDC A,", ["dir"]), 0x40: ("ORL A,", ["dir"]),
          0x50: ("ANL A,", ["dir"]), 0x60: ("XRL A,", ["dir"]),
          0x70: ("MOV", ["dir", "imm"]), 0x80: ("MOV", ["dir", "dir"]),
          0x90: ("SUBB A,", ["dir"]), 0xA0: ("??A5", []),
          0xB0: ("CJNE A,", ["dir", "rel"]), 0xC0: ("XCH A,", ["dir"]),
          0xD0: ("DJNZ", ["dir", "rel"]), 0xE0: ("MOV A,", ["dir"]),
          0xF0: ("MOV", ["dir", "A"])},
}


def d(x):
    return "0x%02X" % x


def disasm(data, start, end):
    out = []
    pc = start
    while pc < end:
        a = pc
        op = data[pc]
        pc += 1
        col = op & 0x0F
        page = op & 0xF0
        if col == 0x1:
            tgt = (pc & 0xF800) | ((op >> 5) << 8) | data[pc]
            pc += 1
            out.append("%04X: %-14s %s 0x%04X" % (a, data[a:pc].hex(),
                                                   "ACALL" if (op & 0x10) else "AJMP", tgt))
            continue
        if col in (0x6, 0x7):
            r = col - 0x6
            m = {0x00: ("INC", ["@R%d" % r]), 0x10: ("DEC", ["@R%d" % r]),
                 0x20: ("ADD A,", ["@R%d" % r]), 0x30: ("ADDC A,", ["@R%d" % r]),
                 0x40: ("ORL A,", ["@R%d" % r]), 0x50: ("ANL A,", ["@R%d" % r]),
                 0x60: ("XRL A,", ["@R%d" % r]), 0x70: ("MOV", ["@R%d" % r, "imm"]),
                 0x80: ("MOV", ["dir", "@R%d" % r]), 0x90: ("SUBB A,", ["@R%d" % r]),
                 0xA0: ("MOV", ["@R%d" % r, "dir"]), 0xB0: ("CJNE", ["@R%d" % r, "imm", "rel"]),
                 0xC0: ("XCH A,", ["@R%d" % r]), 0xD0: ("XCHD A,", ["@R%d" % r]),
                 0xE0: ("MOV A,", ["@R%d" % r]), 0xF0: ("MOV", ["@R%d" % r, "A"])}[page]
        elif col >= 0x8:
            n = col - 0x8
            m = {0x00: ("INC", ["R%d" % n]), 0x10: ("DEC", ["R%d" % n]),
                 0x20: ("ADD A,", ["R%d" % n]), 0x30: ("ADDC A,", ["R%d" % n]),
                 0x40: ("ORL A,", ["R%d" % n]), 0x50: ("ANL A,", ["R%d" % n]),
                 0x60: ("XRL A,", ["R%d" % n]), 0x70: ("MOV", ["R%d" % n, "imm"]),
                 0x80: ("MOV", ["dir", "R%d" % n]), 0x90: ("SUBB A,", ["R%d" % n]),
                 0xA0: ("MOV", ["R%d" % n, "dir"]), 0xB0: ("CJNE", ["R%d" % n, "imm", "rel"]),
                 0xC0: ("XCH A,", ["R%d" % n]), 0xD0: ("DJNZ", ["R%d" % n, "rel"]),
                 0xE0: ("MOV A,", ["R%d" % n]), 0xF0: ("MOV", ["R%d" % n, "A"])}[page]
        else:
            m = COLS[col][page]
        mnem, kinds = m[0], list(m[1])
        s = mnem
        if mnem == "AJMP" or mnem == "ACALL":
            tgt = (pc & 0xF800) | ((op >> 5) << 8) | data[pc]
            s += " 0x%04X" % tgt
            pc += 1
        elif kinds and kinds[0] == "a16":
            s += " 0x%04X" % ((data[pc] << 8) | data[pc + 1]); pc += 2
        else:
            toks = []
            for k in kinds:
                if k == "imm":
                    toks.append("#" + d(data[pc])); pc += 1
                elif k in ("dir", "bit"):
                    toks.append(d(data[pc])); pc += 1
                elif k == "rel":
                    v = data[pc]
                    off = v if v < 128 else v - 256
                    toks.append("0x%04X" % ((pc + 1 + off) & 0xFFFF)); pc += 1
                else:
                    toks.append(k)
            s += " " + ",".join(toks) if toks else ""
        out.append("%04X: %-14s %s" % (a, data[a:pc].hex(), s))
    return out


if __name__ == "__main__":
    path, start, end = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3], 16)
    data = open(path, "rb").read()
    print("\n".join(disasm(data, start, end)))
