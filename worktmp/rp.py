#!/usr/bin/env python3
"""Tiny helper to talk to a RTLPlayground switch over HTTP (no redirects)."""
import sys, http.client

HOST = sys.argv[1] if len(sys.argv) > 1 else "192.168.10.247"
SID = None


def _req(method, path, body=None, ctype=None):
    conn = http.client.HTTPConnection(HOST, 80, timeout=20)
    headers = {}
    if ctype:
        headers["Content-Type"] = ctype
    if SID:
        headers["Cookie"] = "session=%s" % SID
    conn.request(method, path, body=body, headers=headers)
    r = conn.getresponse()
    data = r.read().decode(errors="replace")
    conn.close()
    return r.status, r.getheader("Set-Cookie"), data


def login(pwd="1234"):
    global SID
    st, sc, body = _req("POST", "/login", "pwd=%s" % pwd,
                        "application/x-www-form-urlencoded")
    if sc:
        for part in sc.split(";"):
            part = part.strip()
            if part.startswith("session="):
                SID = part.split("=", 1)[1]
    return st, SID


def get(path):
    st, _, data = _req("GET", path)
    return st, data


def cmd(c):
    st, _, data = _req("POST", "/cmd", c)
    return st, data


if __name__ == "__main__":
    st, sid = login()
    if not SID:
        print("login failed: status", st)
        sys.exit(1)
    args = sys.argv[2:]
    if not args:
        print(get("/information.json")[1])
    for a in args:
        if a.startswith("/"):
            st, d = get(a)
            print("== GET %s [%s]" % (a, st))
        else:
            st, d = cmd(a)
            print("== CMD %s [%s]" % (a, st))
        print(d)
