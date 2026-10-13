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
- Console
  - `defaults`: restores the configuration compiled into the image and reboots. Deliberately not a
    configuration command, so it never becomes part of the startup configuration.
- Devices
  - `MACHINE_PCB_K0402W_V1` — Hisource Hi-K0402WS (PCB-K0402W-V1.0, 4 x 2.5GBASE-T + 2 x SFP+).
    Needs `FLASH_NO_DIO=1`. See doc/devices/PCB-K0402W-V1.0.md.
  - `MACHINE_PCB_K0801W_V1` — Hi-K0801WS / SFP-J08E-HG1 (PCB-K0801W-V1.0, 8 x 2.5GBASE-T + 1 x SFP+).
    Boots and lists its ports correctly, but the LED wiring and the SFP pins are still inherited from
    the V2.0 board and not measured, so it is not listed as supported. Needs `FLASH_NO_DIO=1`.
    See doc/devices/PCB-K0801W-V1.0.md.
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