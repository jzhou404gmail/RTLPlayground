# PCB-K0402W-V1.0 (Hisource Hi-L0402WS / 众通源)

**Status: FULLY VERIFIED on hardware — listed in [doc/supported_devices.md](../supported_devices.md).**
Machine target: `MACHINE_PCB_K0402W_V1` (see `machine.h`), built with `FLASH_NO_DIO=1`
(this board's flash chip needs it — see below).

Verified on a real board: the firmware boots, detects the SoC (RTL8372), initialises the
switch, comes up on 192.168.10.247, the four front-panel LAN ports map to physical ports
1-4, the copper LEDs show orange = 2.5G and green = 1G/100M/10M + activity (both seen with
real links, including a 2.5G peer), and both SFP cages report their module over the
inherited detect/LOS pins with the module EEPROM readable over I2C. The only field that
cannot be verified on this board is `high_leds`: the front-panel power LED is hard-wired
and stays lit even with the system-LED bits cleared.

## Flash chip quirk: DIO MMIO reads fail, build with FLASH_NO_DIO=1

This board carries a **Boya/BoHong BY25Q16BS** (RDID `68 40 15`, 2 MiB). With the default
firmware image the board did not boot: it printed

```
Initializing Flash controller
Detecting CPU: RTL8372
```

and then reset every ~10 ms, forever. The reason is that the 16 KiB HOME area is
prefetched into code RAM by the ROM at power-up (so it runs fine), while every fetch
from a code bank goes through the flash controller's MMIO window, which the default
`flash_init(1)` configures for **Dual-I/O fast read (`0xBB`, mode byte 0x18, 4 dummy
cycles, divider 4)** — and this flash chip returns garbage for that mode. The chip itself
is fine: the written image was read back with two independent programmers and matched
byte for byte, and the flash works in single-bit mode (which is what the later
`Flash information:` probe uses to read the RDID).

Building with single-bit fast read (`0x0B`, 8 dummy cycles, divider 8) fixes it:

```bash
make MACHINE=PCB_K0402W_V1 FLASH_NO_DIO=1
```

The fix is a build-time option rather than a change of the default, because the
Dual-I/O path is an optimisation that works on the other supported boards.

## Boot log on this board (after the fix)

```
Initializing Flash controller
Detecting CPU: RTL8372
Software version: v0.1.0-0170943-dirty
Hardware: PCB-K0402W-V1.0
NIC reset
Setting MAC to: ...
init_switch called
CPU revision: 6868
leds_setup called / leds_setup done
rtl8372_init called ... rtl8372_init done
Flash information: Manufacturer ID: 0x68  Memory Type: 0x40  Capacity: 0x15 = 2 MB
Configuring I2C for SFP idx=00 SCL=03, SDA=04 / idx=01 SCL=03, SDA=03
Verifying PHY settings:
 Port 6 NO SFP Down / Port 1..4 On Down / Port 5 NO SFP Down
Setting ip: 192.168.10.247
```

`regget 0004` returns `0x83720000`, i.e. the SoC is an **RTL8372** (byte 1 = 0x72, not
0x73) — the "RTL8371B" in the stock firmware's banner is the Realtek SDK's name for this
silicon generation, not a different, unsupported chip. `gpio` and `leds` console commands
work, so the remaining parameters can be measured in-circuit without any extra hardware.


V1.0 is the predecessor of the documented [PCB-K0402WS-V2.0](PCB-K0402WS-V2.0.md) and
[PCB-K0402WS-V3.0](PCB-K0402WS-V3.0.md) boards: all three carry the same silkscreen
family (`PCB-K0402W-Vx.y` / `DIP-K0402WS-Vx.y`), and RTLPlayground names them
`PCB_K0402WS_*` after the retail model.

## Board overview (from PCB photos)

**Board markings**
- Top silkscreen: `PCB-K0402W-V1.0` / `DIP-K0402WS-V1.0`
- Date code: `2023-07` (V2.0/V3.0 boards are newer revisions of the same layout)

**Key components**
- `U2`: SPI NOR flash, **8-pin, 2 MiB** — BeyoMicro `25Q16BSSIG` (16 Mbit). A single
  flash chip, unlike the V3.0 board which has two with a selector switch.
- Switch SoC is **under the heatsink**, so the part marking (RTL8371B / RTL8372 /
  RTL8373) is not visible in the photos and has to be read from the boot console.
- Power section: `MC-Tech NT48302S` with `2R2` inductors.
- Ports: 4 × RJ45 2.5GBASE-T + 2 × SFP+ (same chassis/port layout as the V2.0/V3.0
  boards: SFP left, 4 × RJ45, SFP right).
- `QC PASS 24` sticker, same as the sibling revisions.

## Programming

Using a SPI clamp in-board is the only method for the initial installation (the stock
firmware is an unmanaged Realtek reference firmware with a factory-test console, no
web interface, so the web-upgrade path of the OEM installer is not available).

For flashrom/IMSProg the built image must be padded to the chip size:

```bash
make MACHINE=PCB_K0402W_V1
truncate -s 2097152 output/PCB_K0402W_V1/rtlplayground-*-PCB_K0402W_V1.bin
```

Do **not** use the padded file for web flashing or for a RTLPlayground web upgrade.

## Stock firmware dump analysis

A programmer dump of an untouched board (`Hi-L0402WS-20250527.bin`, 2 MiB, sha256
`6d73dcd9a30fc3d735b16b7ceba4caa06c82dc3e26b69f8879229f4be56876e0`) was analysed:

| Property | Stock image | RTLPlayground image | Verdict |
| --- | --- | --- | --- |
| Flash size | 2 MiB (`25Q16BS`) | 512 KiB image, pad to 2 MiB | compatible |
| Boot header | `00 40` (prefetch 0x4000) | `00 40` | identical contract |
| Layout | common code at 0x0002, banks at 0x4000/0x10000 (3 banks) | same scheme, 4 banks | identical contract |
| Used range | 0x00000 – 0x1369A, rest `0xFF` | 0x00000 – 0x7FFFF | — |
| Factory data | none (no MAC/calibration stored in flash) | — | nothing to preserve |
| Firmware | Realtek reference/diag firmware, `RTL8371B:` prompt | RTLPlayground | — |

The layout contract matches the description in [doc/ghidra.md](../ghidra.md), so the
boot ROM will accept an RTLPlayground image written at flash offset 0. The stock
firmware is byte-for-byte the same firmware family as the one seen on the Keeplink
KP-9000-6XH-X (same console prompt, same `?` factory-test menu), i.e. the same
"RTL8371B" silicon generation that RTLPlayground already supports.

**What the dump cannot provide:** the board-specific LED/GPIO wiring. A scan of the
stock image finds **zero** writes of any 0x65xx switch-register address
(`MOV DPTR,#0x65xx` and `MOV direct,#0x65` patterns), whereas RTLPlayground's own build
contains 70 such sites; the stock SDK evidently reaches the LED/GPIO registers through a
different, computed access path. `.led_mux`, `.led_sets`, `.port_led_set` and the SFP
GPIO pins therefore cannot be read out of the flash dump and must be measured.

## Console / chip identification

The stock firmware was confirmed on this board over TTL at **9600 8N1**: it boots to the
prompt `RTL8371B:` and offers the factory-test menu (`rst`, IOL test modes, SDS test
modes, `chip sel: 8373/8372: 3, 8224: 4`). That output is byte-for-byte the same firmware
as in the flash dump (`RTL8371B start ` at offset 0x32A6, `RTL8371B:` at 0x2D91), which
confirms the dump is a genuine read of this board.

The `RTL8371B` banner is a **string compiled into the stock firmware** (the Realtek SDK
build target name), not something RTLPlayground reads. RTLPlayground identifies the SoC
by reading `RTL837X_REG_CHIP_ID` (register 0x0004) and looking at byte 1: `0x73` means
RTL8373, anything else is treated as RTL8372 (`rtlplayground.c`, `Detecting CPU:` /
`INCORRECT MACHINE!`).

The same "RTL8371B" stock firmware runs on the Keeplink KP-9000-6XH-X, a board family
that is already supported, and every 4-copper/5-copper board in `machine.c` — including
that Keeplink — is defined with `isRTL8373 = 0`; only the 8-copper boards are
`isRTL8373 = 1`. A 4 x 2.5G + 2 x SFP+ board therefore belongs to the same class as the
already-supported siblings.

Definitive checks, in order of preference:

1. **Without flashing:** read register 0x0004 over I2C (SoC slave address `0x5C`) with the
   stock firmware running — `tools/led_gpio_utils/i2c_dump_rtl_regs.py` and look at the
   line starting `0004:`.
2. **After flashing RTLPlayground:** the first console lines print `Detecting CPU: RTL8372`
   or `RTL8373`; a mismatch with the machine definition additionally prints
   `INCORRECT MACHINE!`.

## First boot: what each outcome means

RTLPlayground does not compare chip *names* — it reads register 0x0004 and branches
between two init paths only. The first console lines (115200 8N1) therefore decide
everything:

| Console output | Meaning | Action |
| --- | --- | --- |
| `Detecting CPU: RTL8372`, no warning | chip is the supported 4-PHY/2-SerDes class | continue with port/LED bring-up |
| `Detecting CPU: RTL8373` + `INCORRECT MACHINE!` | chip is the 8-copper class | set `isRTL8373 = 1` in the definition and rebuild |
| no output at all / no ports come up | chip cannot be driven by this firmware | restore the 2 MiB backup with the programmer |

The stock firmware's factory menu (`rst`, IOL test modes, SDS test modes) contains no
register read/write and no firmware-update command, so it can neither dump the LED
configuration nor flash anything; the SPI programmer is the only way in.

## Console evidence gathered from the stock firmware

Read directly from the board's UART (`/dev/ttyUSB0`, 9600 8N1) with
`tools/console_capture.py`:

| Test | Result | Interpretation |
| --- | --- | --- |
| `?` | full factory menu (1509 bytes) | console accessible; menu is the complete command set |
| 13 candidate hidden commands (`help`, `i2c`, `reg`, `rd`, `rreg`, `ee`, `pse`, `phy`, `ver`, `info`, …) | all unknown (echo + prompt) | **no register read/write command exists** — LED/GPIO config cannot be read from this console |
| `dump` → `3` → `0` and → `1` | both channels: `pre_en=0x1; pre_amp=0x1; main_en=0x1; main_amp=0x0; main_boost=0x1; post_en=0x1; post_amp=0x1; tx_z0=0x4` | **both SerDes exist and are initialised** → 2 SFP slots, matches `.n_sfp = 2` |
| `dump` → `4` (RTL8224) | no data | **no external RTL8224 PHY** → the 4 copper ports use the SoC's internal PHYs, i.e. the 4-PHY/2-SerDes (RTL8372) profile, not the 8-copper + 8224 (RTL8373) profile |
| cold boot (real power cycle) | output is exactly `\n\nRTL8371B start \n` | this board's ROM does **not** print the `SERDES_*` / register-triple diagnostics seen on the KP-9000-6XH-X, so the boot log carries no additional board configuration |
| `rst` (chip reset) | prints `chip reset` / `RTL8371B start` only | the diagnostics above come from the ROM at cold boot, not from this command |

Note when talking to this console: hold the port open **read-write**. With a read-only
fd the switch's RX input floats and the firmware echoes a prompt for every noise byte,
flooding the log with `RTL8371B:` lines.

Everything that is still unknown (`.led_mux`, `.led_sets`, `.port_led_set`, SFP
`pin_detect`/`pin_los`, exact port order) therefore has to come from either an I2C
register dump of the running stock firmware (`tools/led_gpio_utils/`) or from
RTLPlayground's own console once it is flashed — its `regget`, `regset`, `leds` and
`gpio` commands allow the same values to be derived by experiment, and `regget 0x0004`
settles the RTL8372-vs-RTL8373 question directly.

## Bring-up: record the stock LED behaviour BEFORE flashing

Once RTLPlayground is flashed, the stock firmware's LED/GPIO register values are gone,
and this board's UART console cannot read registers (see the section above). The cheap
way to get equivalent ground truth is to write down what the *stock* LEDs do — it is
then a one-to-one comparison against the candidate configurations (`PCB_K0402WS_V2` and
`PCB_K0402WS_V3`, which differ in exactly these fields):

| Scenario | Which LED lights (position + colour) |
| --- | --- |
| RJ45 1, link at 2.5G | |
| RJ45 1, link at 1G | |
| RJ45 1, link at 100M | |
| RJ45 1, no link, activity only | |
| RJ45 4 (right-most copper) at 1G | |
| SFP left with module inserted, link up | |
| SFP right with module inserted, link up | |
| SFP cage empty (any LED?) | |
| System LED at idle / after boot | |

Also note the front-panel LED count and colour per port (the board is sold as
4 x 2.5G + 2 x SFP+; the siblings have two LEDs per copper port, amber = 2.5G and
green = 1G/100M/10M).

After flashing, the same table is filled in again. Identical behaviour ⇒ the
configuration is right. Differences point at `.led_sets`/`.port_led_set` (which
condition lights which LED) versus `.led_mux` (which pad drives which physical LED).

## LED behaviour and how it was verified

Each copper port has two LEDs side by side: **left = orange, right = green**. The stock
firmware lights orange for a 2.5G link and green for 1G/100M/10M, which is exactly the
semantics of the inherited `PCB_K0402WS_V2.0` LED set:

```
SET 0 (copper): id0 = LEDS_2G5  | LEDS_LINK | LEDS_ACT     -> orange
                id1 = 0
                id2 = LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT -> green
                id3 = 0
SET 1 (SFP):    id0 = all speeds | LEDS_LINK | LEDS_ACT
```

Verified in two steps:

1. **Observation** — with a 1G link on a front port the *green* LED is on and it blinks
   while pinging through the port (so `id2` and its LED pad are correct).
2. **Register experiment** — with the same 1G link, `LED1_0_SET0` was set to `0x00000174`
   (id0 = the 1G condition) and `LED3_2_SET0` to `0`:

   ```
   regset 6548 00000174     # LED1_0_SET0: id0 = 1G|100M|10M|LINK|ACT
   regset 6544 00000000     # LED3_2_SET0: id2 = id3 = 0
   ```

   The green LED went dark and the **orange** LED lit up, proving that the orange LED is
   driven by `LED-id0` of SET0, i.e. that it will light at 2.5G with the stock-like
   definition. The original values were restored afterwards (`0x00000141` / `0x00000174`)
   and read back to confirm.

So for the copper ports `.port_led_set`, `.led_sets` and `.led_mux` all match this board.
Only the SFP LED set and the system LED (`high_leds`) remain to be checked.

## SFP verification (left slot done)

With a module in the left cage and a link up, the console reports it through the inherited
pin assignment (detect GPIO30, LOS GPIO37, I2C SDA4/SCL3):

```
> sfp
Slot 1 - Rate: 67  Encoding: 06
JDSU            PLRXPLSCS43HW   2
Temp: 25e9  Vcc: 8076  TX Bias: 07d8  TX Power: 1784  RX Power: 190b
Slot 2 - empty
```

`gpio` follows: inserting the module flips bit 30 (GPIO30) and the LOS bit (GPIO37), and
`sds` changes from `0x00000bff` to `0x0000035f` once the link is established. The module's
LED lights green (SFP LED set = SET1). The right cage uses GPIO50/51 and SDA3/SCL3 and
still needs a module for confirmation.

## What is assumed vs. what is verified

The initial definition copies the wired parameters from `PCB_K0402WS_V2.0` (the nearest
documented revision, and single-flash like V1.0).

| Parameter | Status |
| --- | --- |
| Flash size / boot compatibility | **verified** (dump analysis above) |
| 4 × copper + 2 × SFP+, no external PHY | **verified** (photos, port layout, sibling defs, `dump` on the stock console) |
| Flash chip needs `FLASH_NO_DIO=1` | **verified** (boot loop → fixed → boots, reproducible build) |
| `isRTL8373 = 0` (RTL8372) | **verified** — boot prints `Detecting CPU: RTL8372`, `regget 0004` = 0x83720000 |
| Both SFP slots and 4 copper ports present | **verified** — boot prints `Port 1..4 On`, `Port 5/6 NO SFP` |
| SFP I2C pins (SDA4/SCL3 and SDA3/SCL3) | **verified for the left slot** — `sfp` reads the EEPROM of a JDSU PLRXPLSCS43HW (vendor, part number, Temp/Vcc/TX Bias/TX Power/RX Power); the right slot still needs a module |
| `log_to_phys_port` / `phys_to_log_port` | **verified** — cable in front LAN 1/2/3/4 brought up physical port 1/2/3/4 respectively (checked on the console with `port <phys> show`) |
| SFP `pin_detect` / `pin_los` (GPIO30/37, GPIO50/51) | **left slot verified** — inserting a module flips bit 30 (GPIO30) in the `gpio` dump and makes `sfp` report the module; GPIO37 (LOS) changes with the signal. Right slot (GPIO50/51) still to be confirmed |
| `high_leds` mux/enable | **not applicable** — the only non-port LED on the panel is a hard-wired power LED: it stays lit even after clearing the system-LED bits (17:16) of `RTL837X_REG_LED_MODE`, and a module is unaffected. No SoC-driven system LED was found, so the inherited value is harmless |
| `led_sets` / `port_led_set` / `led_mux` | **verified for the copper ports** (orange = 2.5G, green = 1G/100M/10M + activity, both confirmed on hardware incl. a real 2.5G link); the SFP LED set (SET1) lights green with a linked module ✓ |

## How to finish the definition (bring-up procedure)

1. **Read the stock LED configuration without flashing anything** — the SoC exposes an
   I2C slave (address `0x5C`) and the sibling FNS-1200P board has an unpopulated SOP8
   footprint wired to it. With an I2C dongle attached to the running *stock* firmware:

   ```bash
   python3 tools/led_gpio_utils/i2c_dump_rtl_regs.py -b <bus> > reg_dump.txt
   python3 tools/led_gpio_utils/dec_leds_from_dump.py
   ```

   The script prints ready-to-paste `.led_mux`, the four LED sets and the per-port set
   assignment. Those values are the ground truth for `machine.c`.

2. **Find the SFP pins** — run `tools/led_gpio_utils/i2c_read_rtl_gpio.py` (or, once
   RTLPlayground runs, the `gpio` console command) and insert/remove a module into each
   cage; the GPIO that changes is that slot's `pin_detect`. `pin_los` needs a module
   without a signal.

3. **Confirm the port order** — link up one RJ45 port at a time and watch the port
   status (`port` console command / web UI); repeat for the two SFP cages.

4. **Confirm the chip** — on the RTLPlayground console the first lines print
   `Detecting CPU: RTL8372` (or `RTL8373`). `INCORRECT MACHINE!` means the
   `isRTL8373` value in the definition is wrong.

5. Update the parameters in `machine.c`, rebuild, and only then move the board from
   "experimental" to supported.

## Serial console

Stock firmware: **9600** 8N1 (as on the V2.0/V3.0 boards). RTLPlayground: **115200** 8N1.
The unpopulated switch footprint (`S1`-like, three through-holes) is wired to UART0;
on the siblings the leftmost hole is RX (GPIO32_UART0_RX), the middle one GND and the
rightmost TX (GPIO31_UART0_TX). Verify on this revision before trusting it.

## Rollback

Keep the 2 MiB dump. Restore it with the same programmer and the board is back to stock —
the analysis above shows nothing outside the first 0.5 MiB is used by the stock firmware,
and the unused area is erased (`0xFF`), so a full-chip restore is a faithful rollback.
