#!/usr/bin/env python3
"""Print the passwd_hash line for a switch password.

The firmware stores only a salted SHA-256 digest and does not implement
SHA-256 on the 8051.  The Web UI hashes in the browser; for a switch that is
reachable only over the serial console, run this on the host and paste the
printed line into the console:

    python3 tools/passwd_hash.py 'correct horse battery staple'
    passwd_hash 0123456789abcdef0123456789abcdef <64 hex digits>

The line is persisted by the firmware immediately (the console loop services
the pending auth write), so no "save" command is needed.

Exit status is 0 on success, 2 on invalid input.
"""
import argparse
import hashlib
import re
import secrets
import sys


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Print a passwd_hash <salt> <digest> line for RTLPlayground."
    )
    ap.add_argument(
        "password",
        nargs="?",
        help="admin password, 1-20 characters (prompted for when omitted)",
    )
    ap.add_argument(
        "--salt",
        help="32 hex characters (16 bytes); a random salt is generated otherwise",
    )
    args = ap.parse_args()

    password = args.password
    if password is None:
        import getpass

        password = getpass.getpass("Password: ")
    if not 1 <= len(password) <= 20:
        print("error: the password must be 1-20 characters", file=sys.stderr)
        return 2
    if len(password) < 6:
        print(
            "warning: the Web UI asks for at least 6 characters for a new password",
            file=sys.stderr,
        )

    if args.salt is None:
        salt = secrets.token_bytes(16)
    else:
        if not re.fullmatch(r"[0-9a-fA-F]{32}", args.salt):
            print("error: --salt must be exactly 32 hex characters", file=sys.stderr)
            return 2
        salt = bytes.fromhex(args.salt)

    digest = hashlib.sha256(salt + password.encode("utf-8")).hexdigest()
    print(f"passwd_hash {salt.hex()} {digest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
