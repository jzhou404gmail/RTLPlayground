#!/usr/bin/env node
/*
 * test_web_auth.js - the browser-side password protocol.
 *
 * The password change posts "passwd_hash <32 hex salt> <64 hex digest>".
 * The digest is SHA-256(salt || password) and must be exactly 64 hex chars:
 * wrapping sha256Hex() in bytesToHex() turns it into 128 chars and makes the
 * command longer than the firmware's 128-byte command buffer. That regression
 * shipped once; this test holds the line.
 *
 * Run: node test/test_web_auth.js
 */
"use strict";

const fs = require("fs");
const path = require("path");
const vm = require("vm");
const crypto = require("crypto");
const assert = require("assert");

const appPath = path.join(__dirname, "..", "html", "app.js");
const src = fs.readFileSync(appPath, "utf8");

const start = src.indexOf("function sha256Hex");
const end = src.indexOf("function api", start);
assert(start >= 0 && end > start, "password helpers not found in app.js");
const block = src.slice(start, end);

const webcrypto = crypto.webcrypto;
const context = {
	Uint8Array,
	DataView,
	Uint32Array,
	Promise,
	TextEncoder,
	Error,
	window: { crypto: webcrypto },
	crypto: webcrypto,
};
vm.createContext(context);
vm.runInContext(block, context);

function isHex(s, n) {
	return typeof s === "string" && s.length === n && /^[0-9a-f]+$/.test(s);
}

(async function main() {
	assert(!/bytesToHex\(sha256Hex/.test(src),
		"pwHash must not wrap the 64-char sha256Hex string in bytesToHex");

	const h = await context.pwHash("123456");
	assert(isHex(h.salt, 32), `salt must be 32 hex chars, got ${h.salt.length}`);
	assert(isHex(h.digest, 64),
		`digest must be 64 hex chars, got ${h.digest.length}: the command buffer would overflow`);

	const want = crypto.createHash("sha256")
		.update(Buffer.from(h.salt, "hex"))
		.update("123456")
		.digest("hex");
	assert.strictEqual(h.digest, want, "digest must be SHA-256(salt || password)");

	const cmd = "passwd_hash " + h.salt + " " + h.digest;
	assert(cmd.length <= 110, `passwd_hash command is too long: ${cmd.length}`);
	assert(cmd.length < 128, "command must fit the firmware's 128-byte buffer");

	// The login page has its own copy of sha256Hex; it must produce the same
	// digest for the salt fetched from /auth.json.
	const login = fs.readFileSync(path.join(__dirname, "..", "html", "login.html"), "utf8");
	const lstart = login.indexOf("function sha256Hex");
	const lend = login.indexOf("function hexToBytes", lstart);
	assert(lstart >= 0 && lend > lstart, "login.html sha256Hex not found");
	const lctx = { Uint8Array, DataView, Uint32Array };
	vm.createContext(lctx);
	vm.runInContext(login.slice(lstart, lend), lctx);
	const salt = Buffer.from(h.salt, "hex");
	const buf = new Uint8Array(salt.length + Buffer.byteLength("123456"));
	buf.set(salt, 0);
	buf.set(Buffer.from("123456"), salt.length);
	assert.strictEqual(lctx.sha256Hex(buf), want,
		"login.html and app.js must compute the same digest");

	console.log("web auth OK: salt 32 hex, digest 64 hex, command " + cmd.length + " B");
})().catch(function (e) {
	console.error("web auth FAIL:", e.message);
	process.exit(1);
});
