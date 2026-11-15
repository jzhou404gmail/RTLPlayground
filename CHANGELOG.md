# Change Log

## [0.1.9] - 2026-11-16

### Fixed

- Web UI password change: `pwHash()` wrapped the 64-character SHA-256 hex string in `bytesToHex()` a second time, producing a 128-character "digest". The resulting `passwd_hash <salt> <digest>` line exceeded the firmware's 128-byte command buffer, so changing the password failed with `ERROR: Command too long`. The digest is now the 64-character `sha256Hex()` result. `test/test_web_auth.js` checks the 32-hex salt, the 64-hex digest, the command length and that `app.js` and `login.html` compute the same value. This supersedes the v0.1.8 image; the rest of the v0.1.8 fixes are included unchanged.

## [0.1.8] - 2026-11-16

### Fixed

- Web UI: the **Log out** button is back in the top-right of the header. It was dropped when the upstream single-page app replaced the fork's UI; the restored button sends `X-RTLP: 1` and posts to `/logout`, which the CSRF guard requires.
- Password change generates a fresh random 16-byte salt for every change again. The upstream UI had reused the salt stored in the firmware, unlike the fork's previous UI.
- The local configuration-command patterns (`passwd_hash`, `http port`/`http tls`, `vlan switch`) are back in `app.js`. Without them the config editor warned about them, the Save merge could duplicate `passwd_hash`, and the dashboard VLAN switch did not mark the config dirty.
- A freshly flashed, fail-closed board can be provisioned from the web again: `/auth.json` reports `legacy`, and `login.html` switches between first-run setup (choose the password, the browser generates the salt and the SHA-256 digest, `POST /login` with `X-RTLP`), digest login, and the old plaintext fallback. The 6-20 character policy is enforced again.
- Serial `passwd_hash ...` is written to the startup configuration. The pending auth write was only serviced by the HTTP command path, so a digest entered on the console lived in RAM until the next boot.
- A malformed `passwd_hash` no longer half-overwrites the live salt/hash: `auth_parse_hex()` decodes into scratch first.
- `auth_persist()` refuses to rewrite a configuration sector that has no NUL terminator (it would otherwise replace the whole configuration with just the auth line), and it strips bare `passwd`/`passwd_hash` lines as well.
- `run_cmd_body()` answers 500 when the auth record could not be written, so the Web UI no longer reports a password change that will disappear at the next reboot.
- A machine-free `make` goal (`clean`, `tools`, `pre-sync-check`, ...) no longer treats every `#define MACHINE_*` in `machine.h` as a machine name; a bare `make` prints the target help again instead of failing with bogus `PCB_INFO`/`MIN_PORT` rules.
- README: the "no password, open the page" and "`passwd <text>` sets a password" text is corrected to the fail-closed first-run flow.

### Added

- `tools/passwd_hash.py`: prints `passwd_hash <salt> <digest>` for a password, for boards reachable only over the serial console.
- `test/test_auth.c`: host-side coverage for the digest parse/compare, `/auth.json`, the configuration write-back, the no-terminator guard and the logout reply.
- `tools/sync-guard.sh` with `make pre-sync-check` / `make post-sync-check`: refuses a dirty upstream sync, backs up local commits as a tag plus patches/diff, and after the sync checks that the logout button, salt rotation, first-run setup, legacy fallback, `passwd_hash` flow and host tests are still present. See `doc/upstream-sync-guard.md`.

## [0.x] - 2026-xx-XX

## Added

- The dashboard has the front-panel VLAN switch card back: it shows the physical switch position and lets the feature be switched on or off (`vlan switch on|off`). `/information.json` carries `model` and the switch fields again, and the System tab shows the retail model next to the PCB revision.
- `/information.json` reports `uptime` (seconds since boot) and the dashboard shows it as days and hh:mm:ss in every language.
- The web session idle timeout is 1 h instead of 200 s, so switching tabs no longer logs the operator out.
- The image builder enforces region limits (`fileadder -l`), so an oversized HTML or configuration payload fails the build instead of overwriting the next region.

- RSTP (802.1w) with the reworked Spanning Tree page, LACP (802.3ad) with `lag <n> lacp <ports>`, the `port <n> cabletest` command, and a per-port learned-MAC count in `/status.json` (`macs`).
- LLDP transmits immediately when it is switched on or a permitted port comes up (four one-second frames, then the 30 s interval), and every frame carries the 802.3 MAC/PHY, 802.1 Port VLAN ID, 802.3 Link Aggregation and Maximum Frame Size TLVs for that port.
- The upstream machine tree is taken as-is (vendor/model names, shared `machine/pcb/` definitions, symlinks, ~150 boards); the fork's six measured boards keep their own `machine/<NAME>.c/.h` files and are built the same way. `make MACHINE=<NAME>` works for both, and a symlinked board reports its PCB revision as `pcb_ver`.
- The board definitions moved to one file pair per board under `machine/` (`machine/<NAME>.c` + `.h`, optional `_init.c`). `MACHINE_CHIPSET` in the header selects the port range. RSTP and the Spanning Tree page live in a fourth code bank (`BANK4`).
- The web UI is the upstream single-page app, which matches the merged JSON schema, with two local rules kept: every POST carries `X-RTLP: 1`, and a password change sends `passwd_hash <salt> <digest>` computed in the browser.

- Four web logins at once (`sessions.c`, in BANK3): each browser has its own cookie, idle timeout and 8 h absolute lifetime; the fifth login replaces the least recently used. A logout ends only the calling browser, a password change ends them all.
- `test/test_sessions.c` covers the session table on the host: distinct 128-bit ids, idle and absolute expiry, LRU eviction, logout scope.
- Build: `CONFIG=<file>` selects the embedded startup configuration, `OUTPUT_DIR`/`OUTPUT_PREFIX`/`OUTPUT_NAME`/`OUTPUT` place the image, `make bank_size` prints how full each code bank of the last build is. BANK1 is the tightest bank (about 88% used on the current tree), so check it before adding code.

## Changed

- DHCP client: renew at most every 30 min, restart with DISCOVER after a DHCPNAK, a failed renewal or lease expiry, and renew immediately when a port comes up. A router that rebooted or upgraded without its lease database sees the switch again without a power cycle; the 30 min cap also covers switches behind another switch, where the uplink does not bounce.
- The main loop is timer-driven: the one-second tick, the SFP poll, the STP steps, LLDP's transmit timer and the LACP periodic timer are timers instead of tick counters. SFP wake-up, DHCP, DNS/NTP waits, the boot-time RTL8224 wait and the reset-button timing follow the same model. The fork keeps its RAM second counter (`read_timer()`), the tick-based press timing that works on RTL8372, the per-model fallback address and the front-panel VLAN switch poll.
- Web API reshape (upstream #768): `/status.json` is gone; `/ports.json` carries the per-port configuration and `/traffic.json` the counters. `/mtu.json` and `/lacp.json` are removed (LAG state is in `/lag.json`), `/vlanlist` is now `/vlanlist.json`, and the JSON uses physical port numbers (CPU = 0). The web UI is the upstream single-page app that speaks this schema; the fork's `X-RTLP` header and browser-side password digest are kept.
- The timer library moved out of `uip/` to `timer.c/h`, in the home bank so every bank can call it; deadlines are 16 bit and `delay()`/`sleep()` each own their timer. The HTTP request deadline is a timer too, and `clock_time()` rereads the tick counter until its low byte stops moving.
- HTTP request handling buffers a POST header across TCP segments, re-examines the byte that breaks a partial multipart boundary, writes the JSON array closers once per array, and answers `/reset` with an empty 200 and resets only after the answer is acknowledged (also when the client's FIN carries the ACK). The upload reset intent lives in the connection state, so a reused TCP slot cannot reset the switch.
- The build date in the System tab is the wall clock at build time again, so an image shows when it
  was actually made. Pass `SOURCE_DATE_EPOCH=<unix time>` for a reproducible build.
- Login throttling: five failed attempts delay the next try by 1 s, doubling up to 32 s. The Web UI
  requires 6-20 characters without spaces for a new password.
- Passwords are stored as a salted SHA-256 digest (`passwd_hash <salt> <digest>`). The browser fetches
  the salt from `/auth.json`, hashes the password locally and posts only the digest; the 8051 never sees
  the plaintext and does not run SHA-256. `passwd` no longer accepts a plaintext argument; a bare
  `passwd` still disables web management. An old plaintext `passwd` line is replayed as a fallback
  until it is removed from the startup configuration.
- `POST /logout` and a Log out button in the header: the session is cleared and the cookie expired.
- Fixed a missing comma in the new logout i18n entry that made `app.js` fail to parse, leaving the
  dashboard blank after login (`v0.1.7`).
- Fixed an 8-bit loop counter in the configuration write-back verification; a configuration longer than
  255 bytes could hang the CPU (found on one of our test units and recovered with a power cycle).
- Web hardening: `X-Frame-Options: DENY`, CSP `frame-ancestors 'none'`, `X-Content-Type-Options:
  nosniff`, `Cache-Control: no-store` on the dynamic endpoints, `Access-Control-Allow-Origin: *`
  removed, the `Host` header must name the switch or the configured hostname (421 otherwise), and
  state-changing POSTs require the `X-RTLP: 1` header (the bundled UI sends it). Scripts that POST to
  `/cmd`, `/config` or `/upload` need to send it too.
- `http tls on|off`: tell the web server it is behind a TLS terminator, so the session cookie gets
  `Secure`. The switch itself still speaks HTTP; terminate TLS on the management host.
- `rtl837x_igmp.c` moved to BANK3 and the response headers were trimmed to keep the image inside
  BANK1.
- Factory defaults on every machine target: the embedded `config.txt` and `config-xikestor.txt` now hold
  `ip dhcp`, so a freshly flashed board obtains its management address over DHCP instead of coming up
  at `192.168.10.247` / `192.168.10.12`. If no DHCP server answers, the firmware stays on its built-in
  fallback, which now matches the model's nameplate: `192.168.10.12/24` for the three XikeStor targets
  (`XIKESTOR_SKS3200M_8GPY1XF`, `SKS3200M_4GPY2XF`, `SKS3200M_5GPY1XF`) and `192.168.10.247/24` for every other
  target. The fallback gateway is `192.168.10.1`; a lease replaces the address as soon as one arrives.
- The web interface is fail-closed: an empty admin password no longer authenticates every request.
  `POST /login` only issues a session for a non-empty, matching password, so a board without a password
  has to be provisioned over the serial console (`passwd <text>`) before its web interface is usable.
  A bare `passwd` now disables web management instead of opening it. Sessions are 128 bit, carry
  `Path=/; HttpOnly; SameSite=Strict`, expire after 8 h regardless of activity, and changing or
  clearing the password invalidates the current session.
- Docs: the 8-port XikeStor target was written as `SKS3200M_8GPY1XF` in README and here, but the
  machine file (and therefore `make MACHINE=...`) is `XIKESTOR_SKS3200M_8GPY1XF`; the old name made
  make stop with "no rule to make target". Fixed in README 1.1/2.5/4.1/5 and in this file, and the
  V2.0 note now says `MOKERLINK_ZX_SWTGW218AS` instead of the no-longer-existing `SWTGW218AS`.
  README 4.2 now states that the three one-shot OEM installers are per model (each embeds that
  model's firmware) and how to build them locally.

## Added

- Web UI
  - Compress the embedded assets (minify + gzip) and switch to a single-page layout. #315
  - Replace the multi-page UI with a themed single-page app: light, dark and Selenized themes following the browser by default,
    English, Japanese and Chinese, save to flash merges the command log into the startup config and verifies the write,
    firmware images are checked in the browser before upload. #429
  - *Restore factory defaults* button in the System tab: writes the configuration compiled into the image
    over the startup configuration and reboots the board, the same as a long press of the reset button.
  - The System tab states the board revision and the retail model of the running target on two lines
    (`.machine_name` / `.model_name`), so a board sold under a different model name than its silkscreen
    says is not mistaken for another device.
  - Open switches are told apart by their browser tab: the dashboard names the tab (and the nav bar)
    after the hostname, and the login page after the hostname seen at the last login on that address,
    the address itself before the first one. While a hostname is unknown - the name has just been
    cleared, the request failed, or the image reports none - the dashboard falls back to the name this
    browser saw at that address last, the model, and then the address, so no tab is left as "Switch".
  - The Time card is translated into Chinese, like the rest of the System tab: the NTP strings had only
    English entries, so a Chinese UI showed that one card in English.
  - The dashboard is a 2 x 2 grid on boards that have a front-panel "VLAN" switch: the port strip and a
    new VLAN switch card share the top row (equal width, equal height), the System and Traffic cards the
    bottom row. The card carries two switches in the style of the spanning-tree port configuration: a
    read-only one showing the panel switch and a note under it, and one turning the feature on or off.
    The page re-reads `/information.json` every five seconds, so the read-only switch follows the panel;
    boards without the switch keep the port strip full width and the card hidden. The VLANs and the
    Isolation tab carry a yellow notice while the panel-switch function is enabled - whatever position
    the physical switch is in: the function owns the LAN ports, so VLAN forwarding does not happen
    between them until it is turned off.
    Everything below the notice is greyed out and read-only in that state - the VLAN table and the
    isolation matrix stay readable, but the controls are disabled so no setting can be changed behind
    the panel switch's back. The **Port mirroring** page gets the same notice and lock, because a
    mirror between two RJ45 ports is exactly the forwarding the isolation drops.
- Console
  - `vlan switch [on|off]`: whether a front-panel VLAN switch drives port isolation. `off` hands VLANs
    and isolation back to the configuration (the firmware clears the isolation once); `on` is the
    default and makes the panel switch decide. Saved like any other configuration command.
  - Command reference in the Console card: a fold-out list of the main console commands, their format and
    one example each, in English and Chinese, so the console does not need the README at hand.
  - The Network card sets the TCP port of the web interface (`HTTP port`, default 80): the field sits
    with the address settings, Apply moves the listener at once, the page redirects itself to the new
    address, and the port is written to the startup configuration with the rest of the network settings.
    A session that is already open keeps working, so the answer to the command still arrives.
- Console
  - `defaults`: restores the configuration compiled into the image and reboots. Deliberately not a
    configuration command, so it never becomes part of the startup configuration.
  - `http [port <1-65535>]`: shows or moves the TCP port the web interface listens on. 0 and values
    above 65535 are refused, and the listener stays where it was.
- Devices
  - `MACHINE_PCB_K0402W_V1` — Hisource Hi-K0402WS (PCB-K0402W-V1.0, 4 x 2.5GBASE-T + 2 x SFP+).
    Needs `FLASH_NO_DIO=1`. See doc/devices/PCB-K0402W-V1.0.md.
  - `MACHINE_PCB_K0801W_V1` — Hi-K0801WS / SFP-J08E-HG1 (PCB-K0801W-V1.0, 8 x 2.5GBASE-T + 1 x SFP+,
    2 MiB Boya/BoHong BY25Q16BS). Boots and lists its ports correctly; the LED wiring and the SFP pins
    are the V2.0 values, not measured on this revision. Needs `FLASH_NO_DIO=1`.
    See doc/devices/PCB-K0801W-V1.0.md.
  - `MACHINE_F7005_2_5` — NPA AP-7024-2.5G (F7005-2.5GPOE-V1.0, 4 x 2.5GBASE-T + 2 x SFP+, RTL8372,
    512 KiB T25S40). Measured LED pad map (2.5G green, 1000/100M orange; the RTL8372 reset value leaves
    LED pads 0-7 off), measured SFP detect pins (GPIO38 left, GPIO37 right) and EEPROM buses
    (SDA1/SCL1 left, SDA0/SCL0 right). The 512 KiB part cannot use the web updater, which stages at
    `0x80000` and needs >= 1 MB; the unit was also brought up by replaying the LED and SerDes commands
    from the startup configuration. See doc/devices/F7005-2.5GPOE-V1.0.md.
  - `MACHINE_STEAMEMO_IG204_V1` — IG204 4+2 on the PB-2131 PCB (4 x 2.5GBASE-T + 2 x SFP+, RTL8372,
    stock 4 MiB cFeon EN25QE32A), now named like the other multi-brand targets: `.machine_name`
    is the board revision (`PB-2131`, shown as *Hardware*) and `.model_name` the label on the unit
    (`IG204 V1`, shown as *Model*). The SFP pins and both cages were verified on the unit
    (GPIO30/GPIO50 detect, SDA4/SCL3 and SDA3/SCL3 EEPROMs, 10G on both), and so was the LED map:
    the green lamp no longer also lights at 2.5G, pads 0-7 are driven to a never-true signal instead
    of being left floating, and `machine_custom_init()` enables every LED pad. The front-panel
    **VLAN slide switch** (GPIO36) is handled too: while it is on, the RJ45 ports are isolated from
    each other but keep their path to the SFP cages and the CPU port. `sys_led_blink_idle` makes the
    SYS LED blink slowly while the board is idle, as the shipped manual describes ("SYS 慢闪 = 正常
    工作"). The manual's claims are tabulated against the implementation in the device document.
    See doc/devices/STEAMEMO_IG204_V1.md.
  - `MACHINE_SKS3200M_5GPY1XF` — XikeStor SKS3200M-5GPY1XF (PCB-SWTG115AS-V2.1, 5 x 2.5GBASE-T + 1 x SFP+),
    the ZX-SWTGW215AS board under a different model name. See doc/devices/SKS3200M-5GPY1XF.md.
  - `MACHINE_XIKESTOR_SKS3200M_8GPY1XF` — XikeStor SKS3200M-8GPY1XF (SWTG118AS-V2.1-17462, 8 x 2.5GBASE-T + 1 x SFP+),
    the ZX-SWTGW218AS board under a different model name.
  - `MACHINE_SKS3200M_4GPY2XF` — XikeStor SKS3200M-4GPY2XF (SWTG024AS-v1.0, 4 x 2.5GBASE-T + 2 x SFP+),
    the SWTG024AS board under a different model name. Enables the reset button and the factory MAC for
    this unit only; see doc/devices/SWTG024AS.md.
  - Factory default configuration per target: the XikeStor builds embed `config-xikestor.txt` and
    therefore come up at 192.168.10.12 instead of 192.168.10.247.
- Tools
  - `tools/console_capture.py`: timestamped serial capture that needs no pyserial.
  - `tools/reset_button_probe.py`: watches the reset button pin and the uptime over HTTP, so a short
    press, a reboot and a factory reset can be told apart while the button is pressed by hand.
- Build
  - `FLASH_NO_DIO=1`: selects single-bit fast read instead of the flash controller's Dual-I/O MMIO read
    mode, for SPI NOR parts that return garbage in that mode (Boya/BoHong BY25Q16BS and relatives).
    Without it, those boards reset in a loop right after `Detecting CPU`.

## Changed

- Web UI
  - The System tab reports the hardware and the model of the target, see above.
- Upgrade
  - Documented that a firmware upgrade keeps the board's startup configuration: the image is written up
    to the configuration area, which is left alone, so VLANs, port names, the password and the IP address
    survive an upgrade. The configuration compiled into the image only applies to a board that has none
    yet, or after restoring the factory defaults.

## Fixed

- SFP ports are reported as enabled with a separate `module` flag instead of using `enabled` for "module inserted", so an empty cage shows as *not connected* rather than *disabled*; the port details name the missing module.
- `send_l2()` counted 76 bytes for a worst-case entry but the entry can reach 96, and the closing bracket was written without checking the buffer; the host test caught the overrun.
- Firefox no longer fills a saved login into the new-password fields of the System tab.
- NTP
  - An NTP server given by host name never synced on a board that has no DNS server: the client sent
    nothing, `ntp`/`time` only said *Time not synchronised* and the Time card showed `Server address -`,
    which reads like a server that does not answer rather than a look-up that was never made. That is
    what a board configured with a static address and without a `dns server` line is - as the factory
    default configuration is. The client now names the reason on the console and in the Time card:
    `no DNS server for <name>`, `cannot resolve <name>`, `no usable answer from <address>`, `waiting for the
    first answer`, or `NTP is off`. An IPv4 address in the NTP server field was, and is, used as it is,
    without a look-up.
  - On an RTL8372 whose `RTL837X_REG_SEC_COUNTER` reads back 0 the clock stayed at the value it had at
    the last synchronisation and the client never scheduled the next one: the uptime was taken from
    that register, which only holds a written value on an RTL8373. It now comes from the RAM counter
    `read_timer()` keeps - the same one the session timeout uses - so `time`, the Time card and the
    dashboard clock advance on those units too.
- Config
  - A startup configuration line longer than the command buffer no longer stops the replay:
    the offending line is skipped and the following lines are still applied.
  - The web UI and the configuration upload now refuse a line the replay cannot take,
    instead of writing it to flash.
- Reset button
  - `handle_button()` measured the press length from `RTL837X_REG_SEC_COUNTER`. That register only holds
    a written value on the RTL8373; on an RTL8372 it always reads back 0, so every press looked like a
    short press and the button did nothing at all on those boards. The press is now timed from the
    software tick counter, which `boot.c` already used for the boot-time press.
- HTTP
  - The session timeout derived its deadline from the same seconds register, so on an RTL8372 a session
    cookie stopped being accepted a few seconds after login. Deadlines now come from the tick counter,
    which this fix makes correct on both chip generations.
- Devices
  - The XikeStor SKS3200M-4GPY2XF came up with a generated MAC instead of the one its stock firmware
    showed. `.mac_flash_offset = 0x1FC000`, set for this target, makes the board use its factory MAC
    again (verified on the unit).
  - The XikeStor units are identified by the model name on their label and in their own web UI, not by
    the name an earlier revision of the device list assumed.
  - The SWTG024AS family crossed over its two SFP jacks in the logical port numbering: on the board
    jack 5 (`J4`, left) is 10GMAC 8 on SerDes 1 and jack 6 (`J2`, right) is 10GMAC 3 on SerDes 0, but
    the machine table had jack 5 on logical port 3 and jack 6 on logical port 8. A module in jack 5
    therefore came up - and lit its lamp - as jack 6, while a module in jack 6 was not seen at all.
    `.log_to_phys_port` / `.phys_to_log_port` follow the port overview now, and the SFP slots, their
    SerDes, their detect/RX-LOS pins and their lamps follow their jacks. This affects every variant of
    the family, including the V2.0 boards that upstream supports.
  - The XikeStor SKS3200M-4GPY2XF reported only one SFP cage. The target now describes both: jack 5
    (`J4`, SFP1) is logical port 8 - detect GPIO30, RX-LOS GPIO37, I2C SDA4/SCL3, SerDes 1 - and jack 6
    (`J2`, SFP2) is logical port 3 - detect GPIO50, RX-LOS GPIO51, I2C SDA3/SCL3, SerDes 0.
  - The XikeStor SKS3200M-4GPY2XF lit the wrong lamps: it inherited the LED pad map of the SWTG024AS-V2.0
    board, on which the lamps sit on other pads. Ports and colours were crossed (a 1000M link lit the
    right-hand green - the 2.5G - lamp, LAN3 lit LAN4's lamp), both SFP lamps stayed dark and several
    ambers glowed without a link. The target now carries the pad map measured on the unit, its SFP cages
    get one lamp lit at any speed, and the pads of unused signals are driven to their inactive level
    instead of floating. See the "LEDs" section of doc/devices/SWTG024AS.md.
  - The 2.5G lamp of the IG204 4+2 and of the NPA AP-7024-2.5G stayed solid while data flowed: their
    LED mask was `LEDS_2G5 | LEDS_LINK` without `LEDS_ACT`, so at 2.5G - the speed both boards normally
    run at - the lamp (amber on the IG204, green on the AP-7024) was lit but never blinked. Both targets
    set the bit now, like the SWTG024AS and Hi-K0801WS targets do, and the manual lists
    "闪烁:数据传输中" for the port lamp. Verified on the IG204: the pad level of RJ45 4's amber lamp
    toggles during a ping flood, stops toggling when the bit is cleared live with `regset 6548`, and
    toggles again once it is restored.

## Breaking changes

- Config
  - VLAN don't accept port `u`-suffix anymore.
    So `vlan 1 4u` is not valid anymore.
    Replace it with `vlan 1 4`.
- Commands
  - port zero/`0` is treated as the `CPU_PORT`. #326
  - Many commands don't accept the CPU_PORT anymore. See #334.
    When the CPU_PORT is needed, the command/service will add the CPU_PORT automaticly.
    Only `isolate` accept the CPU_PORT as destination port.

## [v0.1_aplha] - 2025-11-03

First release.