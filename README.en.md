# RTLPlayground — device-support fork

**English** · [中文说明](README.md) · upstream project: [logicog/RTLPlayground](https://github.com/logicog/RTLPlayground)

This is a fork of [logicog/RTLPlayground](https://github.com/logicog/RTLPlayground). It carries the
upstream firmware plus hardware support that upstream does not have yet:

- **six machine targets** for boards that were unsupported or listed under a wrong model name, plus the
  upstream `MACHINE_STEAMEMO_IG204_V1` rewritten from measurements instead of a placeholder,
- **the fixes those boards needed** — a reset button that never did anything on RTL8372 silicon, a web
  session that stopped being accepted seconds after login on the same silicon, and a factory MAC the
  board stopped using,
- **one feature they depend on** — restoring the factory configuration without a reset button, from the
  web interface or the console,
- **documentation and tools** for the next bring-up: device pages with board photos and measurements,
  a serial capture and a reset-button probe.

Everything on this page was measured on real hardware (eight switches, 2026-10-11 … 2026-10-29).
"Verified" always means *measured on the board*, and the device document quotes the command output or
the observation that settles it.

| | |
|---|---|
| Firmware version | `v0.1.0`, with the commit stamped in — `version` on the switch reports it; the builds these measurements were taken with report `v0.1.0-e976582`. A build from a clean checkout carries no `-dirty` suffix |
| Based on upstream | `main` at [`03311de`](https://github.com/logicog/RTLPlayground/commit/03311de) |
| License | MIT, same as upstream — see [LICENSE](LICENSE) |
| Full device list | [doc/supported_devices.md](doc/supported_devices.md) |
| Upstream instructions | kept unchanged at the [bottom of this file](#upstream-readme-unchanged) |

## Contents

1. [Which target do I build?](#1-which-target-do-i-build)
2. [Device notes](#2-device-notes) — build command, install path and verified state per board
3. [What else this fork changes](#3-what-else-this-fork-changes)
4. [Recipes](#4-recipes) — build, install, first boot
5. [Verification](#5-verification) — the eight units, and how to check your own board

## 1. Which target do I build?

### 1.1 The six targets this fork adds, plus the IG204 rewritten from the unit

| Board silkscreen | Sold as | Managed | Ports | SoC | Flash | Machine target | Build |
|---|---|---|---|---|---|---|---|
| `PCB-K0402W-V1.0` / `DIP-K0402WS-V1.0` | Hisource **Hi-K0402WS** (众通源) | no | 4 × 2.5G + 2 × SFP+ | RTL8372 | 2 MiB Boya/BoHong `BY25Q16BS` | `PCB_K0402W_V1` | **needs `FLASH_NO_DIO=1`** |
| `PCB-K0801W-V1.0` / `DIP-K0801WS-V1.0` | Hisource **Hi-K0801WS** (unit labelled `SFP-J08E-HG1`) | no | 8 × 2.5G + 1 × SFP+ | RTL8373 | 2 MiB Boya/BoHong `BY25Q16BS` | `PCB_K0801W_V1` | **needs `FLASH_NO_DIO=1`** |
| `PCB-SWTG115AS-V2.1-17443` | XikeStor **SKS3200M-5GPY1XF** (兮克) | yes | 5 × 2.5G + 1 × 10G SFP+ | RTL8372 | 2 MiB Winbond `W25Q16JV` | `SKS3200M_5GPY1XF` | `make MACHINE=…` |
| `SWTG024AS-v1.0` | XikeStor **SKS3200M-4GPY2XF** | yes | 4 × 2.5G + 2 × SFP | RTL8372 | 2 MiB Winbond `W25Q16JV` | `SKS3200M_4GPY2XF` | `make MACHINE=…` |
| `SWTG118AS-V2.1-17462` | XikeStor **SKS3200M-8GPY1XF** | yes | 8 × 2.5G + 1 × SFP+ | RTL8373 | 2 MiB Winbond `W25Q16JV` | `SKS3200M_8GPY1XF` | `make MACHINE=…` |
| `PB-2131` | IG204 4+2 (label `IG204 V1`, sold by Steamemo and others) | no | 4 × 2.5G + 2 × SFP+ | RTL8372 | stock 4 MiB cFeon `EN25QE32A`; the measured unit carries a 1 MiB `MX25L8005M` | `STEAMEMO_IG204_V1` | `make MACHINE=STEAMEMO_IG204_V1 FLASH_NO_DIO=1` |
| `F7005-2.5GPOE-V1.0` | NPA **AP-7024-2.5G** | no | 4 × 2.5G + 2 × SFP+ | RTL8372 | stock 512 KiB BrightMoon `T25S40`; the final unit carries a 2 MiB `BY25Q16BS` | `F7005_2_5` | `make MACHINE=F7005_2_5 FLASH_NO_DIO=1` |

The three XikeStor units are not new boards: the PCB inside them is a board upstream already supports
(the ZX-SWTGW215AS, the SWTG024AS family and the ZX-SWTGW218AS). The new targets exist because those
boards are **sold under a different model name**, and the firmware should say so in the web interface —
plus, on the 4GPY2XF, because the button and the factory MAC needed enabling for that retail variant.

The IG204 4+2 (`PB-2131`) and the NPA AP-7024-2.5G (`F7005-2.5GPOE-V1.0`) *are* new boards. The
upstream `MACHINE_STEAMEMO_IG204_V1` was a placeholder that had never been measured - both SFP detect
pins, the LED pads and the front-panel VLAN switch were wrong or missing - and `F7005_2_5` did not
exist. Both were brought up item by item; see their sections in part 2.

### 1.2 Four upstream device-list entries this fork corrects

| Upstream says | The unit actually is | Effect |
|---|---|---|
| `XikeStor SKS3200-8E1X` on SWTG118AS-V2.1-17462 | **`SKS3200M-8GPY1XF`** — stated on the case label and in the unit's own web interface | you would have looked for the wrong model, and the firmware reported a name the unit does not carry |
| Hisource Hi-K0801WS is on `PCB-KO801W-V2.0` | **`PCB-K0801W-V2.0`** — the silkscreen has a digit `0`, not a letter `O` | the target and the document were renamed accordingly; the old spelling no longer exists anywhere in the tree |
| `Hisource Hi-K0402WS` on `PCB-K0402W-V1.0`: not listed | added, supported, 2 MiB flash | a whole board revision was missing |
| The Steamemo IG204 4+2 board is named `Steamemo IG204 V1`, all in one line | hardware **`PB-2131`** and model **`IG204 V1`**, on two lines - the same PB-2131 board is sold under several brands | the upstream target carried no measured SFP pins, no measured LED map and no panel-switch support; this fork fills all three in |

Also corrected: the brand spelling `Xikestore` → `XikeStor`, the 4GPY2XF flash size (`?` → 2 MiB), and
the flash size and PCB label of the 8GPY1XF row.

### 1.3 Identifying your board in three steps

1. **Open the case and read the silkscreen revision** — a `PCB-…` string printed near an edge, e.g.
   `PCB-K0402W-V1.0`, `PCB-SWTG115AS-V2.1-17443`, `SWTG024AS-v1.0`. The accompanying `DIP-…` marking is
   the same revision of the panel-side board.
2. **Check the case label**: model name and port count. It tells you which retail variant you have, not
   which PCB revision — a Hi-K0402WS exists on V1.0, V2.0 and V3.0 boards that need *different* targets.
3. **Match the silkscreen, not the retail name.** If the silkscreen matches a row above, use that target.
   If it does not, look it up in [doc/supported_devices.md](doc/supported_devices.md) — upstream covers
   most of the other revisions (KP-9000, SWTG024AS-V2.0, ZX-SWTGW215AS, ZX-SWTGW218AS, PCB-K0402WS
   V2.0/V3.0, and more).

> [!IMPORTANT]
> Four of the targets above are built and measured with `FLASH_NO_DIO=1`: Hi-K0402WS, Hi-K0801WS, the
> IG204 4+2 and the AP-7024-2.5G. **Hi-K0402WS and Hi-K0801WS definitely need it**: without the flag they
> **reset in a loop right after `Detecting CPU`**. That is not a bricked board and not a wrong target —
> it is the Boya `BY25Q16BS` chip refusing the Dual-I/O read mode. Rebuild with `FLASH_NO_DIO=1` and flash
> again. The IG204 and AP-7024 flash parts (`MX25L8005M`, `T25S40`, `BY25Q16BS`) were only ever run with
> single-bit fast read as well, so keep the flag for them.

## 2. Device notes

### 2.1 Hisource Hi-K0402WS — `PCB-K0402W-V1.0` → `MACHINE_PCB_K0402W_V1`

RTL8372, 4 × 2.5GBASE-T + 2 × SFP+ (1G/2.5G/10G), one 2 MiB Boya/BoHong `BY25Q16BS` (`RDID 68 40 15`),
SoC under a heatsink. **Status: supported — verified on two units.**

| | |
|---|---|
| Build | `make MACHINE=PCB_K0402W_V1 FLASH_NO_DIO=1` |
| Install | SPI clamp only. The stock firmware is an unmanaged Realtek reference image with a factory-test console on **9600 8N1** and no web interface, so the web-upgrade path does not exist for the first install |
| Image for the programmer | pad the 512 KiB build to 2 MiB (`truncate -s 2097152 …`); **never** use a padded image for web flashing |
| Reset button | **none**, and no unpopulated footprint for one — restore the built-in configuration from the web UI or with `defaults` |
| Default IP | `192.168.10.247` (from `config.txt`) |
| Console | 115200 8N1 once RTLPlayground runs |
| Document | [doc/devices/PCB-K0402W-V1.0.md](doc/devices/PCB-K0402W-V1.0.md) |

Verified on the board: boots with `FLASH_NO_DIO=1`, is an RTL8372 (`regget 0004` = `0x83720000`), all six
ports in the right order (one cable per jack), copper LEDs (amber = 2.5G, green = 1G/100M/10M with
activity, including a real 2.5G link), **both** SFP cages (module detect, RX_LOS, module EEPROM over I2C,
10G link), and the *Restore factory defaults* path in the web UI.

### 2.2 Hisource Hi-K0801WS — `PCB-K0801W-V1.0` → `MACHINE_PCB_K0801W_V1`

RTL8373, 8 × 2.5GBASE-T + 1 × SFP+, 2 MiB Boya/BoHong `BY25Q16BS` (`RDID 68 40 15`).
**Status: supported — first bring-up and measurements on 2026-10-12.**

| | |
|---|---|
| Build | `make MACHINE=PCB_K0801W_V1 FLASH_NO_DIO=1` |
| Install | SPI clamp only (same unmanaged stock firmware as above); keep the 2 MiB dump |
| Reset button | **none**, and no unpopulated footprint for one |
| Default IP | `192.168.10.247` |
| Document | [doc/devices/PCB-K0801W-V1.0.md](doc/devices/PCB-K0801W-V1.0.md) |

Verified: it boots with the V1.0 silkscreen, reports the RTL8373 class (no `INCORRECT MACHINE!`), lists
all nine ports in the right places in the web UI, and the flash mode is right (the Boya/BoHong part
needs `FLASH_NO_DIO=1`). The LED sets, the LED pad mux, the system LED and the SFP pins are the V2.0
values — they were not measured on this revision. The document lists the console commands that settle
each of them; the photos show the revision, the date code and the flash chip.

### 2.3 XikeStor SKS3200M-5GPY1XF — `PCB-SWTG115AS-V2.1-17443` → `MACHINE_SKS3200M_5GPY1XF`

RTL8372, 5 × 2.5GBASE-T + 1 × 10G SFP+ (the fifth RJ45 is driven by an external RTL8221B), 2 MiB Winbond
`W25Q16JV` (`EF 40 15`). **Status: supported — all six ports verified.** Same PCB as the Lianguo
ZX-SWTGW215AS; the two targets differ only in the board/model names they report.

| | |
|---|---|
| Build | `make MACHINE=SKS3200M_5GPY1XF` (or `MACHINE=LIANGUO_ZX_SWTGW215AS`, same wiring) |
| Install | keep a full 2 MiB backup first — the stock image cannot be rebuilt from a dump. From the stock web UI: the OEM installer; once any RTLPlayground image runs: the web interface's *Firmware update* |
| Reset button | `GPIO54` — **verified**: < 3 s nothing, ~5 s reboot with the startup configuration kept, ~13 s factory reset |
| Default IP | `192.168.10.12` (the XikeStor targets embed `config-xikestor.txt`) |
| Document | [doc/devices/SKS3200M-5GPY1XF.md](doc/devices/SKS3200M-5GPY1XF.md) |

Verified: port order (UI 1-5 = jacks 1-5, UI 6 = SFP), links at 1G and 2.5G, SFP module detect/RX_LOS,
module EEPROM read and a port linked at 10G, RJ45 LEDs (2.5G right green, 1G/100M left amber), the
system LED, the factory MAC at `0x1FC000`, and the reset button above.

### 2.4 XikeStor SKS3200M-4GPY2XF — `SWTG024AS-v1.0` → `MACHINE_SKS3200M_4GPY2XF`

RTL8372, 4 × 2.5GBASE-T + 2 × SFP, 2 MiB Winbond `W25Q16JV`. **Status: supported — the lamps and both
cages were measured on the unit on 2026-10-13.** Same PCB family as the SWTG024AS-V2.0 boards, but the
two targets are **not** interchangeable: this one has the LED pad map measured on the unit, describes
both cages, and enables the reset button and the factory MAC, which the V2.0 siblings may not have
populated.

| | |
|---|---|
| Build | `make MACHINE=SKS3200M_4GPY2XF` |
| Install (stock → RTLPlayground) | the stock firmware is a CGI-based Realtek SDK image (固件版本 V10.9.4, `admin/admin`, `192.168.10.12`). Its 固件升级 page (`fwug.cgi`) switches the board into the vendor loader, which **keeps the same IP**; the loader then takes the OEM installer via `/httpupg.cgi?cmd=fw_upgrade` |
| Reset button | `GPIO54` (K1) — **verified**: ~5 s reboot, ~11 s factory reset, < 3 s nothing |
| Factory MAC | restored: `mac_flash_offset = 0x1FC000` makes the board report `1C:2A:A3:23:E4:5E` again instead of a generated MAC |
| The two cages | `J4` (left, jack 5) is logical port **8** — SerDes 1, detect GPIO30 / RX-LOS GPIO37, I2C SDA4+SCL3 — and `J2` (right, jack 6) is logical port **3** — SerDes 0, detect GPIO50 / RX-LOS GPIO51, I2C SDA3+SCL3. The two jacks **cross over** in the logical numbering |
| Lamps | pad map measured on the unit: green/amber on pad 12/14 (jack 1), 15/17 (2), 18/20 (3), 21/23 (4), the two cage lamps on pad 24 (`J4`) and pad 9 (`J2`); a cage lamp lights at any speed |
| Default IP | `192.168.10.12` |
| Document | [doc/devices/SWTG024AS.md](doc/devices/SWTG024AS.md) |

> [!NOTE]
> If your router has MAC-based DHCP reservations, re-create them for the factory MAC after flashing this
> target — the generated MAC the board used before is gone for good.

### 2.5 XikeStor SKS3200M-8GPY1XF — `SWTG118AS-V2.1-17462` → `MACHINE_SKS3200M_8GPY1XF`

RTL8373, 8 × 2.5GBASE-T + 1 × SFP+, 2 MiB Winbond `W25Q16JV`. **Status: supported.** Same PCB as the
Mokerlink ZX-SWTGW218AS (V2.0); the target exists to report the board as `PCB-SWTG118AS-V2.1` and the
model as `SKS3200M-8GPY1XF`.

| | |
|---|---|
| Build | `make MACHINE=SKS3200M_8GPY1XF` (or `MACHINE=SWTGW218AS` for the V2.0 boards) |
| Install (stock → RTLPlayground) | the stock firmware is a CGI image (固件版本 V1.9, `admin/admin`, `192.168.10.12`) whose 固件升级 page (`fwug.cgi`) accepts the OEM installer |
| Reset button | `GPIO54` — **verified**: ~5 s reboot, ~13 s factory reset, < 3 s nothing |
| Default IP | `192.168.10.12` |
| Document | [doc/devices/SWTGW218AS.md](doc/devices/SWTGW218AS.md) |

Verified: the button's three states, the session handling, and restoring the built-in configuration from
the web UI.

### 2.6 IG204 4+2 — `PB-2131` (label `IG204 V1`) → `MACHINE_STEAMEMO_IG204_V1`

RTL8372, 4 × 2.5GBASE-T + 2 × SFP+, stock 4 MiB cFeon `EN25QE32A-104HIP` (SOP-8, `U5`), and a **VLAN
ON/OFF slide switch** on the front panel. **Status: supported - ports, lamps, both SFP cages and the
switch were measured on the unit (2026-10-29).** Upstream's target of the same name was a placeholder
(no measured SFP pins, no measured LED map, no panel switch at all); this fork rewrote it from the unit.
The web interface shows **hardware `PB-2131` / model `IG204 V1`**, like the other multi-brand targets.

| | |
|---|---|
| Build | `make MACHINE=STEAMEMO_IG204_V1 FLASH_NO_DIO=1` |
| First install | SPI clip or programmer only: the stock firmware is the unmanaged Realtek reference image (serial **9600 8N1**, prompt `RTL8371B:`, no web interface). The measured unit had its 4 MiB part replaced with a 1 MiB `MX25L8005M` (the 512 KiB image plus the web updater's 1 MB staging area need the room) |
| Already running RTLPlayground | upload `output/STEAMEMO_IG204_V1/rtlplayground-*.bin` (512 KiB) in the web UI's firmware page |
| Reset button | none, not even a footprint → *Restore factory defaults* in the web UI or the `defaults` command |
| Default IP | `192.168.10.247` (`config.txt`) |
| Console | header `T7` (TX/GND/RX/3V3); 115200 8N1 once RTLPlayground runs |
| Panel VLAN switch | `GPIO36`, reads 1 when on and 0 when off; on, the four copper ports are isolated from each other and only reach the SFP+ cages and the CPU port; off, everything talks. Polled once a second, so it takes effect in about a second |
| Document | [doc/devices/STEAMEMO_IG204_V1.md](doc/devices/STEAMEMO_IG204_V1.md) |

Measured: `regget 0004` = `0x83720000` (RTL8372, no external PHY); the four copper ports at 2.5G; copper
lamps **2.5G amber / 1G·100M·10M green**, both colours **blinking with traffic**; **both SFP cages**
(detect GPIO30/GPIO50, EEPROM over I²C, 10G links) with two 10G modules swapped from cage to cage; the SYS
lamp slow-blinking as the manual describes; the panel switch sampled ten times in each position. Every
claim in the shipped manual is tabulated against the implementation in the *Conformance with the manual*
section of the device document.

### 2.7 NPA AP-7024-2.5G — `F7005-2.5GPOE-V1.0` → `MACHINE_F7005_2_5`

RTL8372, 4 × 2.5GBASE-T + 2 × SFP+ (1G/2.5G/10G), stock 512 KiB BrightMoon `T25S40`, 12 V 1 A barrel
plug, no reset button (and no footprint for one). **Status: supported - the copper ports, the lamps and
both SFP cages at 10G were measured on the unit (2026-10-19).** Upstream has no such target.

| | |
|---|---|
| Build | `make MACHINE=F7005_2_5 FLASH_NO_DIO=1` |
| First install | write the 512 KiB image at offset 0 of the `T25S40` with an SPI clip/programmer |
| Web update | **not on a 512 KiB board**: the updater stages the image at `0x80000` and needs at least 1 MB, so the upload answers HTTP 400 `Flash too small for firmware upload!`. With a 2 MiB `BY25Q16BS` (same SOP-8 footprint) the web update works and both cages are detected automatically |
| Reset button | none, `.reset_pin = GPIO_NA` → web UI button or `defaults` |
| Default IP | `192.168.10.247` |
| Console | header `JP1` (VCC/RX/GND/TX); 9600 8N1 on the stock firmware, 115200 8N1 on RTLPlayground |
| Document | [doc/devices/F7005-2.5GPOE-V1.0.md](doc/devices/F7005-2.5GPOE-V1.0.md) |

Measured: `regget 0004` = `0x83727000` (RTL8372); the four copper ports at 2.5G; copper lamps **2.5G green
/ 1G·100M amber**, with the pad map measured one group at a time (RJ45 1 green 3 / amber 4-5, RJ45 2 green
6 / amber 8, RJ45 3 green 9-10 / amber 12-14, RJ45 4 green 18-20 / amber 21-23); `LED_GLB_IO_EN` must be
written `0x7fffffff` or the whole panel stays dark; the cages detect on **GPIO38 (left) / GPIO37 (right)**
and their EEPROMs answer on **SDA1/SCL1 and SDA0/SCL0** (none of them the reference pins) - one 10G module
was moved from cage to cage and read with `sfp` and linked at 10G in both.

## 3. What else this fork changes

### 3.1 Restore the factory defaults without a reset button — new feature

The System tab (*Maintenance*) has a **Restore factory defaults** button, and the console has the
equivalent `defaults` command. Both write the configuration compiled into the image over the startup
configuration and reboot the board, which then comes up at the image's default address (`192.168.10.12`
on the XikeStor builds, otherwise the address in `config.txt`).

It is deliberately **not** a configuration command: it never becomes part of the startup configuration.
The UI text exists in all six interface languages (English, Japanese, Chinese, Spanish, French, German).

Why it matters: the Hi-K0402WS and the Hi-K0801WS have no reset button at all, so before this there was
no way to get the built-in configuration back on those boards. Verified on the two Hi-K0402WS units and
on the 4GPY2XF, 5GPY1XF and 8GPY1XF.

### 3.2 Reset button timing — fix, every RTL8372 board with a button

`handle_button()` measured how long the button was held from `RTL837X_REG_SEC_COUNTER`. That register
only holds a written value on the RTL8373; on an RTL8372 it always reads back 0, so **every press looked
like a short press and the button did nothing at all** on those boards. The press is now timed from the
software tick counter (measured against a wall clock: 200.0 ticks/s, so the thresholds are real seconds):

| Press | What happens |
|---|---|
| < 3 s | nothing |
| > 3 s | the switch reboots, the startup configuration is kept |
| > 10 s | the configuration compiled into the image replaces the startup configuration, then reboot |

Tri-state verified on the SKS3200M-4GPY2XF, -5GPY1XF and -8GPY1XF. This is shared code, so every RTL8372
target with a button benefits — including boards this fork does not otherwise touch.

### 3.3 Web session timeout — fix, RTL8372

The HTTP session deadline was derived from the same seconds register, so on an RTL8372 a session cookie
stopped being accepted a few seconds after login. Deadlines now come from the tick counter, which is
correct on both chip generations (verified on each).

### 3.4 Factory MAC — fix, SKS3200M-4GPY2XF

That unit came up with a generated, locally administered MAC instead of the one its stock firmware
showed. `.mac_flash_offset = 0x1FC000` — set for this target only, because that area is unverified on the
V2.0 siblings — makes the factory MAC come back; `/information.json` reports `1c:2a:a3:23:e4:5e` and the
hostname derives from it again.

### 3.5 Board and model shown separately — new

`struct machine` gained `model_name`. Targets fill in both the PCB revision (`machine_name`, shown as
硬件 / hardware) and the retail model (`model_name`, shown as 型号 / model), so a board that is sold
under a different name than its silkscreen says is no longer mistaken for another device. Targets that
leave `model_name` empty behave exactly as before.

### 3.6 Per-target factory default configuration — new

`config-xikestor.txt` (IP `192.168.10.12`) is embedded instead of `config.txt` for the three XikeStor
targets; the Makefile picks the file per machine target (`DEFAULT_CONFIG`). A freshly flashed XikeStor
board therefore comes up at `192.168.10.12` rather than `192.168.10.247`.

### 3.7 `FLASH_NO_DIO=1` — new build option

Some SPI NOR parts (Boya/BoHong `BY25Q16BS` and relatives, `RDID 68 40 15`) return garbage for the flash
controller's Dual-I/O MMIO read mode. `FLASH_NO_DIO=1` selects single-bit fast read instead, which those
parts handle correctly and which every 3.3 V SPI NOR seen so far also supports. It stays a build option
because Dual-I/O is a speed optimisation that works on the other supported boards.

### 3.8 Build ergonomics

Building without selecting a machine now prints the list of available targets instead of failing
obscurely, and `make machine_check` compiles `machine.c` for every target in `machine.h` (both are also
run by CI).

### 3.9 Tools for the next bring-up

- [`tools/console_capture.py`](tools/console_capture.py) — timestamped serial capture that needs no
  pyserial; useful because the unmanaged boards speak 9600 8N1 in stock firmware and 115200 8N1 once
  RTLPlayground runs.
- [`tools/reset_button_probe.py`](tools/reset_button_probe.py) — polls the button's GPIO level and the
  uptime over HTTP, so a short press, a reboot and a factory reset can be told apart while someone
  presses the button by hand. The method (write a marker hostname into the startup configuration first)
  is written up in [doc/devices/SWTG024AS.md](doc/devices/SWTG024AS.md).

### 3.10 Tests and documentation

`make -C test` (host tests, extended to cover the new model line) and `make machine_check` pass; CI
builds both chip classes, with and without the health instrumentation. Documentation added or corrected:
the seven device pages of section 2 - among them the rewritten
[STEAMEMO_IG204_V1.md](doc/devices/STEAMEMO_IG204_V1.md) and the new
[F7005-2.5GPOE-V1.0.md](doc/devices/F7005-2.5GPOE-V1.0.md) - plus
[doc/devices/SWTGW218AS.md](doc/devices/SWTGW218AS.md) and [doc/devices/SWTG024AS.md](doc/devices/SWTG024AS.md),
board and manual photos for seven models,
[doc/supported_devices.md](doc/supported_devices.md), the RTL8372/RTL8373 seconds-register difference in
[doc/hardware.md](doc/hardware.md), and the note that a firmware upgrade keeps the startup configuration
(section 4.3 below).

### 3.11 The two SFP jacks of the SWTG024AS family cross over — fix, the whole family

The board's own port overview has the left cage `J4` (jack 5) on **10GMAC 8 / SerDes 1** and the right one
`J2` (jack 6) on **10GMAC 3 / SerDes 0**, so the two cages cross over in the logical numbering. The
machine table had them the other way round — jack 5 on logical port 3 and jack 6 on logical port 8 — so a
module in the left cage came up (and lit its lamp) as the right one, while a module in the right cage was
not seen at all. `log_to_phys_port` / `phys_to_log_port` follow the port overview now, and the SFP slots,
their SerDes, their detect/RX-LOS pins and their lamps follow their own jack. This is a property of the
board, so the SWTG024AS-V2.0 boards upstream supports (Lianguo, Horaco, Sodola) are fixed by the same
change.

### 3.12 The SKS3200M-4GPY2XF lamps and its second cage — fix

That unit inherited the LED table of the SWTG024AS-V2.0 board, whose lamps sit on other pads: ports and
colours were crossed (a 1000M link lit the right-hand green — the 2.5G — lamp, LAN3 lit LAN4's lamp),
both cage lamps stayed dark and several ambers glowed with no link at all. The target now carries the pad
map **measured on the unit, one pad at a time** (green/amber on pad 12/14, 15/17, 18/20 and 21/23, the
cage lamps on pad 24 and pad 9), a cage lamp lights at any speed, and the pads of unused signals are
driven to their inactive level instead of floating (a floating pad lights every lamp that is pulled
down). The same unit also got its second cage described, see [2.4](#24-xikestor-sks3200m-4gpy2xf--swtg024as-v10--machine_sks3200m_4gpy2xf).
The measuring method is written up in the "LEDs" section of
[doc/devices/SWTG024AS.md](doc/devices/SWTG024AS.md).

### 3.13 Browser tab names — fix

Upstream named the dashboard tab after the switch hostname, but only once `/information.json` had
answered with one: while the page was loading, after a failed request, or on an image that reports no
hostname, several switches open side by side all went back to sitting there as "Switch". The login page
now names the tab from the name this browser remembered for that address before the first paint (each
switch is its own origin), and the dashboard resolves the name as the live hostname, the remembered name,
the model and finally the address, and writes it to the nav bar as well.

### 3.14 The front-panel VLAN switch — new feature

The slide switch marked **VLAN ON/OFF** (on the IG204 it sits left of the SYS lamp) is handled by the
firmware: `vlan_switch_poll()` samples its GPIO once a second, and while the switch is on it writes the
isolation mask `0x308` for the four copper ports - they are isolated from each other but all still reach
the SFP+ cages and the CPU port, so the management address and the uplink stay reachable. Off writes
`0x3f8` back (everything talks). The registers are only written when the position changes, so flipping
the switch takes effect in about a second and needs no reboot.

The rest of the feature:

- `vlan switch [on|off]` on the console turns the **function** on or off. `on` is the default; with it off
  the panel switch does nothing and VLANs and isolation belong to the web/console configuration again. It
  is saved like any other configuration command (*Save to flash* to keep it).
- The dashboard's **VLAN panel switch settings** card in the top right holds two switches in the style of
  the spanning-tree port configuration: a read-only one showing where the panel switch actually is (with
  a line of text saying what that position means), and one that enables or disables the function (the
  English UI: *VLAN panel switch* and *Use VLAN panel switch*). The card is as wide as the port strip and
  shares its row.
- Mutual exclusion: while **Use VLAN panel switch** is on, the VLANs, Isolation and Port mirroring pages are
  greyed out and read-only behind a yellow notice that says the panel switch owns the LAN ports - whatever
  position the physical switch is in. Turning the function off makes all three editable again. The lock
  keys off the function, not off the switch position, because the position can be flipped by hand at any
  time and a page must not turn read-only in the middle of an edit.

### 3.15 Port lamps blink with traffic — fix, IG204 4+2 and AP-7024-2.5G

Both targets first described their 2.5G lamp as `LEDS_2G5 | LEDS_LINK` and left out `LEDS_ACT`, so **at
2.5G - the speed both boards normally run at - the lamp was solid while data flowed**, and the other
colour stayed dark because the speed did not match: the panel looked as if the lamp never moved. Both
colours carry `LEDS_ACT` now, as on the already verified SWTG024AS and Hi-K0801WS targets, which is what
the manual describes ("闪烁 = 数据传输中").

How it was verified on the IG204: the second word of the `gpio` command is *what changed since the last
call*; with `ping -f` running, RJ45 4's amber pad (21) keeps appearing in it, i.e. the lamp is blinking.
Clearing the `ACT` bit live with `regset 6548 01740041` made the pad stop moving completely, and writing
the value back made it blink again. The SYS lamp was changed to the manual's slow blink while idle
(`sys_led_blink_idle`) in the same pass.

## 4. Recipes

### 4.1 Build

```bash
# Hi-K0402WS (PCB-K0402W-V1.0) and Hi-K0801WS (PCB-K0801W-V1.0): the Boya flash needs this flag
make MACHINE=PCB_K0402W_V1 FLASH_NO_DIO=1
make MACHINE=PCB_K0801W_V1 FLASH_NO_DIO=1

# the three XikeStor units
make MACHINE=SKS3200M_4GPY2XF
make MACHINE=SKS3200M_5GPY1XF
make MACHINE=SKS3200M_8GPY1XF

# IG204 4+2 (PB-2131) and NPA AP-7024-2.5G (F7005-2.5GPOE-V1.0): measured with single-bit fast read
make MACHINE=STEAMEMO_IG204_V1 FLASH_NO_DIO=1
make MACHINE=F7005_2_5 FLASH_NO_DIO=1
```

The image lands in `output/<target>/rtlplayground-<version>-<target>.bin` (512 KiB).

### 4.2 Install

| Your board now runs | Use |
|---|---|
| RTLPlayground | *Firmware update* in the web interface, upload the 512 KiB `output/<target>/rtlplayground-*.bin` |
| stock **managed** firmware (the three XikeStor units) | the one-shot installer `installer/output/rtlplayground_oem_upgrade.bin`, uploaded through the stock 固件升级 page — see the per-device notes in section 2 for the loader differences |
| stock **unmanaged** firmware (Hi-K0402WS, Hi-K0801WS) | an SPI clamp and a programmer: back up 2 MiB, erase, write the build padded to 2 MiB, verify. Padding is only for the programmer — never upload a padded file |
| stock **unmanaged** firmware (IG204 4+2) | an SPI clamp and a programmer: back up the stock 4 MiB `EN25QE32A`, write the 512 KiB image at offset 0. The stock part was never run with RTLPlayground - the measured unit carries a 1 MiB `MX25L8005M` |
| stock **unmanaged** firmware (NPA AP-7024-2.5G) | a 512 KiB `T25S40` takes the unpadded 512 KiB image from an SPI programmer. For the web updater, fit a 2 MiB `BY25Q16BS` (same SOP-8 footprint), see the [device document](doc/devices/F7005-2.5GPOE-V1.0.md) |

### 4.3 After flashing

- A freshly flashed XikeStor board comes up at `192.168.10.12` with the default password **`1234`**; a
  Hi-K0402WS/Hi-K0801WS/IG204 4+2/AP-7024-2.5G board comes up at `192.168.10.247`.
- **An upgrade keeps the board's startup configuration.** The image is written up to the configuration
  area, which is left alone, so VLANs, port names, the password and the IP address survive a firmware
  update. The configuration compiled into the image only applies to a board that has none yet, or after
  restoring the factory defaults.
- Restore the built-in configuration with *Restore factory defaults* in the System tab, with the
  `defaults` console command, or with a > 10 s press of the reset button where the board has one. The
  board then moves to the image's default address, so reconnect there.
- The boot log and the System tab state the board revision and the retail model on separate lines, so
  you can always tell which target a board is running.
- The **VLAN** slide switch on the front panel (left of the SYS lamp on the IG204) is handled by the
  firmware: on, the four RJ45 ports are isolated from each other but all still reach the SFP+ cages and
  the CPU port, so the management address and the uplink keep working; off, everything talks. It is
  polled once a second, so it takes effect in about a second without a reboot, and the *Isolation* page
  shows the corresponding mask. The dashboard's *VLAN panel switch settings* card shows the switch
  read-only and adds an *Use VLAN panel switch* switch: turning that off makes the panel switch do
  nothing and hands VLANs and isolation back to the web configuration (the console equivalent is
  `vlan switch off`; *Save to flash* to keep it across a reboot).
- **Do not connect several links of one switch to several ports of another switch.** STP is off by
  default, so that forms a loop: with RJ45 1-3 and both SFP cages connected to the same switch the IG204
  produced a broadcast storm of about 900k packets/s, the management address stopped answering, and only
  pulling the extra cables recovered it. Use one link between two switches, or run `stp on` and save the
  configuration first.

## 5. Verification

### 5.1 The eight units this work was measured on

| Unit | Target | What was verified on it |
|---|---|---|
| Hisource Hi-K0402WS #1 | `PCB_K0402W_V1` | full bring-up: ports, copper LEDs, both SFP cages, factory-default restore, session |
| Hisource Hi-K0402WS #2 | `PCB_K0402W_V1` | factory-default restore from the UI, session |
| Hisource Hi-K0801WS (`SFP-J08E-HG1`) | `PCB_K0801W_V1` | boots, nine ports correct, flash mode (`FLASH_NO_DIO=1`); LEDs and SFP pins are the V2.0 values, not measured on this board |
| XikeStor SKS3200M-8GPY1XF | `SKS3200M_8GPY1XF` | reset button tri-state, session, factory-default restore |
| XikeStor SKS3200M-4GPY2XF | `SKS3200M_4GPY2XF` | reset button tri-state, factory MAC, factory-default restore; **both cages** (jacks 5/6 = logical ports 8/3), the **pad map measured lamp by lamp**, the port-numbering crossover fixed (2026-10-13) |
| XikeStor SKS3200M-5GPY1XF | `SKS3200M_5GPY1XF` | all six ports, SFP at 10G, LEDs, system LED, factory MAC, reset button tri-state |

All eight were reflashed with the same build and reported the same software version, with their IP address,
hostname and factory MAC preserved.

### 5.2 Checking your own board

On the host, before flashing:

```bash
make -C test          # host unit tests
make machine_check    # machine.c must compile for every target
```

On a running board (console or web interface command line):

```
version        # target, board revision, retail model, build
regget 0004    # byte 1: 0x73 = RTL8373, otherwise RTL8372
gpio           # pin levels — move a module into each SFP cage and watch which bit flips
leds           # dump or force LED sets, to match the panel against the machine definition
sfp            # module presence and EEPROM of each cage
port <n> show  # which logical port a cable lands on
```

For the reset button, write a marker into the startup configuration first (e.g. `hostname RESETTEST`)
and use [`tools/reset_button_probe.py`](tools/reset_button_probe.py): a reboot shows as the uptime
restarting with the marker intact, a factory reset as the marker disappearing.

---

# Upstream README (unchanged)

<!-- Everything below this line is the README of logicog/RTLPlayground at 03311de, verbatim. -->
# RTLPlayground
A Playground for Firmware development for advanced user of RTL8372/RTL8373 based 2.5GBit Switches.

For each hardware configuration of these devices, there is usually a managed and an
unmanaged version sold, with mostly identical hardware. The aim is to provide management
features also for unmanaged devices with additional features such as Management VLAN,
DHCP servers, multi-language support, IPv6 and TLS-encrypted web-pages. At present, however
only the following features are provided:
- A modern web-interface with mouse-over to display further information
- A serial console interface to configure all features
- IGMP to configure Multicast streaming
- Port configuration showing detailed information about own and Link-partner advertised
  Speed settings and configuration of these settings on the local side
- Per-port configuration of frame sizes (MTUs) for Jumbo-Frame support or limiting MTUs
  for particular devices
- EEE (Energy Efficient Ethernet) can be configured per-port. Detailed information is
  provided for support offered by the link partner and the EEE status of a port.
- VLAN configuration
- SFP information is displayed on the inserted modules, the current sensor values such as
  temperatures, RX and TX power are displayed in the CLI and as mouse-over on the web
- Mirror configuration
- Link Aggregation Groups can be set up
- Detailed information on port packet statistics
- Configuration saved to flash via the web-interface
- Firmware updates via the web
- Installation as a firmware upgrade from the original web-interface

<img width="1673" height="977" alt="GUI" src="doc/images/gui.png" />

While the firmware provides already considerable improvements over the original managed firmware,
the firmware still lacks support for the proprietary loop prevention
protocols as well as DHCP. Spanning Tree is available (see doc/stp.md), but is
a simplified implementation - read that document before enabling it on a
switch you administer over the network. If you need these features, do not install the playground on your managed
devices. In any case, installation is strongly discouraged unless you can at least make
a backup of the original flash content via a SOIC clamp such as also used for BIOS
backups and can re-install that firmware in case something is wrong. For this no soldering
skills are necessary.

The firmware supports all hardware features of devices with
- 4 2.5GBit ports + 2 SFP+ ports
- 5 2.5GBIT + 1 SFP+ port
- 8 2.5GBit + 1 SFP+ port
Devices sold usually have a fairly common design, however there may be differences in the LED
configuration (switches have LEDs with different colours and use types of LEDs). The list
of tested devices can be found in [Supported devices](doc/supported_devices.md).

To do meaningful development you will need to use a serial console, so soldering skills
are required. Flashing must be done via a SOIC-8 PatchClamp or by soldering a socket
for the flash chip.

If you don't want to open your device, you can use the project's code to learn about the
devices by looking at the image using e.g. Ghidra. If you want to contribute to the
design of the web-interface or get a feeling for the interface first, a standalone
device simulator is provided, which runs entirely under Linux as a local webserver.

## (0) Compiling Requirements

Install the following particular build requisites (Debian 12/13), note that Ubuntu 24.04
still has an older version of sdcc, but you will need sdcc version 4.5 for the code to compile:
```
sudo apt install make gcc sdcc xxd python-is-python3 libjson-c-dev zlib1g-dev
```

<details>
<summary>If using Docker (click to expand)</summary>

### Prerequisites

Install Docker for your platform:

- **Linux (Debian/Ubuntu)**: `sudo apt install docker.io` then `sudo usermod -aG docker $USER` (log out and back in)
- **Linux (other distros)**: Follow the [Docker Engine install guide](https://docs.docker.com/engine/install/)
- **Windows**: Install [Docker Desktop for Windows](https://docs.docker.com/desktop/setup/install/windows-install/)
- **macOS**: Install [Docker Desktop for Mac](https://docs.docker.com/desktop/setup/install/mac-install/)

### Usage

A Dockerfile is provided for a reproducible build environment:

```
docker build -t rtlplayground-dev .
```

Build the firmware (replace MACHINE with your target, e.g. `DEFAULT_8C_1SFP`):

```
docker run --rm -v $(pwd):/workspace rtlplayground-dev make MACHINE=DEFAULT_8C_1SFP
```

The resulting `.bin` file appears in `output/` on your host.

Build host tools only:

```
docker run --rm -v $(pwd):/workspace rtlplayground-dev make -C tools
```

Run the web-interface simulator locally:

```
docker run --rm -p 8080:8080 -v $(pwd):/workspace rtlplayground-dev \
  tools/output/httpd_sim /workspace/html
```

Edit `machine.h` or `config.txt` on your host, then re-run `make` — the
source directory is mounted into the container, so changes take effect
immediately. To build for a different machine, pass `MACHINE=...`.

</details>

## (1) Compiling for direct chip flashing AND upgrading an existing RTLPlayground running device

Edit machine.h with an editor like vi or nano. Select the correct machine the firmware should build for.

> [!TIP]
> You can write configuration parameters in config.txt (see below) in order your switch to get
> straight at the first boot, a correct IP configuration.

Now, building the firmware image should work:
```
make 
```
Note, that the image generated ends in .bin, not .img, in order to make IMSProg happy.

image location is stored in `RTLPlayground/output/rtlplayground_version_machine.bin`
for example
```
rtlplayground-v0.1.0-12c98ba-dirty-LIANGUO_ZX_SWTGW215AS.bin
```

> [!CAUTION]
> This image can be flashed directly to the chip OR through the firmware update/upgrade
> interface of RTLPlayground interface

## (2) Compiling for OEM running device with management options (web upgrade)

Managed switches can be updated from the existing original firmware using a SPECIFIC upgrade image.
You first need to build the firmware for direct chip flashing : See below (1)

Then

```
cd installer
make 
```
image location is stored in  `RTLPlayground/installer/output/rtlplayground_oem_upgrade.bin`

> [!CAUTION]
> This image must ONLY be used for original OEM firmware web interface firmware upgrade.
> You do not need this image if you are already on RTLplayground firmware.
> Unless you go back to the original OEM firmware, you would only flash this specific firmware
> only once. Future upgrades of RTLPlayground will only need to follow (1)

example of compilation console output

```
RTLPlayground/installer$ make
mkdir -p output
gcc updatebuilder.c -o output/updatebuilder
sdas8051 -plosgff -o output/crtstart.rel crtstart.asm
sdcc -mmcs51 --code-loc 0x1000 -o output/installer.rel -c installer.c
sdcc -mmcs51 -Wl-bHOME=0x1100 -Wl-r -o output/rtlinstaller.ihx output/crtstart.rel output/installer.rel
./output/updatebuilder -i output/rtlinstaller.ihx -o output/rtlplayground_oem_upgrade.bin ../output/rtlplayground.bin
Input file size: 524288
Bytes read: 524288
EOF
Payload sum 1 is: 0x25100
Payload sum 2 is: 0x25100
Payload sum with header is: 0x264ec
Payload sum is: 0xf8fe94
Header checksum is: 0x5a1
```

## (3) Sandbox Usage with Ghidra (optional)

You can play with the image using ghidra or flash real Switch Hardware. For
ghidra see this information about [Ghidra images](doc/ghidra.md).

## (4) Installation through the Web interface (software way)

Managed switches (OEM firmware of RTLplayground firmware) can be upgraded via the web interface.
Unmanaged switch cannot be flashed this way (see 5).

Go to "Firmware update" tab, select the correct file.

> [!IMPORTANT]
> If your device already runs RTLPlayground, you must upload the binary file /RTLPlayground/output/rtlplayground_Version_Machine.bin
> If your device is OEM, you must upload the binary file /RTLPlayground/installer/outputrtlplayground_oem_upgrade.bin

> [!CAUTION]
> Check one more time that your device matches the machine type before flashing.
> Be sure you have a backup of the original firmware before diving in RTLPlayground.

Finally, push the Upload File Button and you're done !


## (5) Flashing the ROM directly (hardware way, but also only way to rescue)

This procedure is the only way to flash unmanaged switches, if the ROM chip is large enough.
This is also the only way to unbrick your device if something went wrong.

> [!IMPORTANT]
> You need a SOIC-8 clip to flash the ROM chip directly onboard.
> Alternatively you can de-solder the flash chip and install a SOIC adapter).
> For flashing the chip directly, you must use the binary file /RTLPlayground/output/rtlplayground_Version_Machine.bin

> [!CAUTION]
> As you need to open your switch case, consider that the warranty is gone.

- Disconnect power from switch.
- Open the switch.
- Attach the clip onto the flash chip (Red line on Pin 1, Pin 1 has a point marker).
- Connect USB of flash programmer, the power LED on the switch will light up, check cabling if not.
- Don't panic, mixing up GND and 3.3V usually does not destroy the switch.
- Use IMSProg, Flashrom, or whatever Programmer to detect the chip.
- MAKE A BACKUP (DUMP) OF THE EXISTING FIRMWARE !
- ERASE THE ROM (BLANK) !
- Load the firmware into IMSProg.
- Flash is to the ROM chip.
- Disconnect the clip from the ROM chip.
- You're done, ready for the first boot.

## (6) Connecting a serial interface (optional)

You can connect a serial cable to the UART port found on all the devices, set 8N1 @ 115200 baud.

## (7) Power Up

When you power up the switch, the device will perform some examples and provide a minimal console
(if wired to a serial interface), the documentation of which can be found in the source code rtlplayground.c`.

## (8) The web-interface

The web-interface can be reached under the [default 192.168.10.247](http://192.168.10.247) unless you
specified an IP address in the config.txt before compilation.

> [!TIP]
> The default password is `1234`.

## (9) The command line

The command line is very rudimentary and mostly for testing purposes.
The following is a boot-log with some examples:
```
Detecting CPU
RTL8373 detected
Starting up...
  Flash controller

NIC reset
rtl8372_init called

RTL837X_REG_SDS_MODES: 0x00000bed

phy_config_8224 called

phy_config_8224 done

rtl8224_phy_enable called

rtl8224_phy_enable done

rtl8372_init done

A minimal prompt to explore the RTL8372:

CPU detected: RTL8373
Clock register: 0x00001101
Register 0x7b20/RTL837X_REG_SDS_MODES: 0x00000bed
Verifying PHY settings:

 Port   State   Link    TxGood          TxBad           RxGood          RxBad
1       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
2       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
3       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
4       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
5       On      2.5G    0x00000008      0x00000000      0x00000000      0x00000000
6       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
7       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
8       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
9       NO SFP  Down    0x00000000      0x00000000      0x00000000      0x00000000

> port 5 1g
  CMD: port 5 1g
PORT 04 1G

> stat
  CMD: stat
 Port   State   Link    TxGood          TxBad           RxGood          RxBad
1       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
2       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
3       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
4       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
5       On      1000M   0x00000035      0x00000000      0x00000017      0x00000000
6       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
7       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
8       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
9       NO SFP  Down    0x00000000      0x00000000      0x00000000      0x00000000

>
<SFP-RX OK>

<MODULE INSERTED>  Rate: 67  Encoding: 01
Lightron Inc.   WSPXG-ES3LC-IHA 0000

> stat
  CMD: stat
 Port   State   Link    TxGood          TxBad           RxGood          RxBad
1       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
2       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
3       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
4       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
5       On      1000M   0x00000065      0x00000000      0x0000003b      0x00000000
6       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
7       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
8       On      Down    0x00000000      0x00000000      0x00000000      0x00000000
9       SFP OK  10G     0x00000000      0x00000000      0x0000001c      0x00000000

> sfp
  CMD: sfp
Rate: 67  Encoding: 01
Lightron Inc.   WSPXG-ES3LC-IHA 0000
```

## (10) Advanced configuration

You can configure more deeply the switch without the need of the console mode.

While in compilation part, you might write directly to config.txt file before making the binary firmware

```
nano config.txt
```

If you want to modify settings after the flash is done, go to the System tab and find the Startup Configuration.

<img width="1673" height="978" alt="ADVANCED SETTINGS" src="doc/images/advanced_settings.png" />

```
ip xxx.xxx.xxx.xxx      = IP address of the switch
gw yyy.yyy.yyy.yyy      = IP address of the gateway
netmask zzz.zzz.zzz.zzz = Network mask of the switch 
port x name xxx         = Name xxx the port number x
port z 1g               = Set 1g speed for port z
igmp on/off             = Turn IGMP on or off
session xxxx            = Web session timeout in seconds (default 200)
```
[To be continue]

Enjoy playing!

## (11) Other documents

The following documents give further documentation on specific features of the RTL837x SoCs:
- [RTL8372/3 Feature support](doc/hardware.md)
- [CPU Port](doc/CpuPort.md)
- [L2 learning](doc/l2.md) 
- [IGMP (IP-MC streaming)](doc/igmp.md)
- [SFP+ ports](doc/sfp.md) 
- [Trunking aka. port aggregation](doc/link_aggregation.md)
- [VLAN](doc/vlan.md)
- [Storm control](doc/storm_control.md)
- [Modifications and Flash replacement](doc/mods.md)
