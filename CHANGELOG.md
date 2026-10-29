# Change Log

## [0.x] - 2026-xx-XX

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
  - `MACHINE_SKS3200M_8GPY1XF` — XikeStor SKS3200M-8GPY1XF (SWTG118AS-V2.1-17462, 8 x 2.5GBASE-T + 1 x SFP+),
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