#!/usr/bin/env node
/*
 * test_web_features.js - fork-specific UI rules that are easy to lose in an
 * upstream SPA sync.
 *
 * - the header Log out button uses the same red "danger" style as Reboot
 * - the Maintenance card explains the factory defaults (DHCP, fallback IP,
 *   default password 1234)
 * - the RESET-button instructions are shown only for the three SKS targets
 *
 * Run: node test/test_web_features.js
 */
"use strict";

const fs = require("fs");
const path = require("path");
const vm = require("vm");
const assert = require("assert");

const root = path.join(__dirname, "..");
const index = fs.readFileSync(path.join(root, "html", "index.html"), "utf8");
const app = fs.readFileSync(path.join(root, "html", "app.js"), "utf8");

assert(index.includes('class="ctl danger" id="logoutBtn"'),
	"Log out must use the same red danger style as Reboot");
assert(index.includes("button.danger"),
	"the danger button style must exist");
assert(index.includes('id="sy-resetnote"'),
	"Maintenance must have a reset-button note element");
assert(index.includes('id="sy-defaults-other"'),
	"Maintenance must have the non-SKS default-parameter note");
assert(index.includes('id="sy-defaults-sks"'),
	"Maintenance must have the SKS default-parameter note");
assert(index.includes("white-space:pre-line"),
	"the factory-default notes must keep the two-line layout");
assert(app.includes("sy_defaults_note_other"),
	"Maintenance must explain the non-SKS factory defaults");
assert(app.includes("sy_defaults_note_sks"),
	"Maintenance must explain the SKS factory defaults");
assert(app.includes("sy_reset_note"),
	"Maintenance must explain the RESET button");
assert(app.includes("RESET 键：") && app.includes("RESET button: hold"),
	"the RESET hint must start with RESET 键/RESET button, not the model name");
assert(!app.includes("SKS 三款带 RESET 键"),
	"the RESET hint must not repeat the SKS model prefix");
assert(app.includes("1234"),
	"the factory-default password must be documented in the UI strings");

const start = app.indexOf("function hasResetButton");
const end = app.indexOf("function sysLoad", start);
assert(start >= 0 && end > start, "hasResetButton() not found in app.js");
const ctx = {};
vm.createContext(ctx);
vm.runInContext(app.slice(start, end) + "\nthis.hasResetButton = hasResetButton;", ctx);

function test(info, want, label) {
	ctx.S = { info: info };
	assert.strictEqual(ctx.hasResetButton(), want, label);
}

test({ model: "SKS3200M-8GPY1XF" }, true, "8GPY1XF (model) has RESET");
test({ hw_ver: "XikeStor SKS3200M-8GPY1XF" }, true, "8GPY1XF (hw_ver) has RESET");
test({ model: "SKS3200M-4GPY2XF" }, true, "4GPY2XF has RESET");
test({ model: "SKS3200M-5GPY1XF" }, true, "5GPY1XF has RESET");
test({ model: "IG204 V1", hw_ver: "PB-2131" }, false, "IG204 has no RESET");
test({ model: "SFP-J08E-HG1", hw_ver: "PCB-K0801W-V1.0" }, false,
	"K0801W has no RESET");

assert(app.includes("192.168.10.247/24"), "the non-SKS note must mention .247");
assert(app.includes("192.168.10.12/24"), "the SKS note must mention .12");
assert(app.includes("password is 1234") || app.includes("密码默认 1234"),
	"the default password 1234 must be documented");
assert(app.includes("\\nWeb") || app.includes("\\nThe web"),
	"the factory-default note must contain its line break");

console.log("web features OK: red logout, split two-line factory notes, SKS-only RESET hint");
