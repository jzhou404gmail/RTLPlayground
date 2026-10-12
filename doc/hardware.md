#RTL8272/3 features

The following hardware features of the RTL8372/3 is supported:
- Clock generation, including different divider settings
- Interrupt control for timer, serial, external irqs 0, 1
- Serial console via SFRs
- Flash operations via SFRs
- Bank switching via SFRs
- Access to Switch registers via SFRs
  - LED setup
  - Reset
  - Some switch settings such as MAC configuration
  - GPIO to detect SFP module insert/removal/RX-LOS (depending on device/module support)
  - I2C to read SFP EEPROM on 1 and 2 SFP slot devices
  - NIC setup
  - L2 learning table access, L2 table flushing
  - VLAN setup/configuration
  - Port mirroring
- Access to PHYs via MDIO (clause 45 via SFR):
  - Internal PHYs of RTL8372 and RTL8373
  - RTL8221 (1x2.5GBit port on devices with 5 ports)
  - RTL8224 (4x2.5GBit ports on devices with 8 ports)
- SerDes settings of SoC via SFR:
  - Configure SFPs with 10Gbit/2.5Gbit/1Gbit (Ethernet and Fiber SFP(+) tested)
  - RTL8221, RTL8224
- NIC TX and RX of packets via SFRs
  - send and receive Ethernet frames via SFRs and Switch registers
  - RTL-tags and VLAN ingress-tag decoding for CPU-port

Ethernet frame RX IRQ via IRQ1 is conceptually understood, but not activated. RX is
currently done via polling, which allows ping-times of <10ms.

The RTL8372/3 have 256 bytes of internal RAM (INTMEM) accessible through MOV
instructions, which are used for the stack and important globals. Some of
these are bit-adressable, e.g. for storing global flags.

Additionally, 64kB of extended RAM (XMEM) is built in, which is accessed
through the MOVX instruction. It is used for global variables, for most
of the function argument passing that is not done using the 8 registers
R0-R7 or registers A/B, and for local variables (which requires extremely
careful planning). The flash memory is transparently accessible for code
being executed and can be used to store configuration. Access is done through
the MOVC instruction, possibly setting the bank register before and
resetting it to access the entire 4MB space. Code is prefetched from flash
and cached in a small RAM automatically by the HW.

The peripherial functions are accessed through 2 different mechanisms:
- Special Function Registers (SFRs, 0x80-0xff) for banking, timers, UART, access to
  switch registers, MDIO, SPI (flash) and NIC transfers. Some SFRs are not
  used for HW purposes and can be used as RAM. Some SFRs are bit-adressable,
  allowing for very tight event wait loops (a single 2-byte instruction).
- 0x10000 switch registers, which appear to be very similar to the registers
  of the RTL838x, for which source code and datasheets are available. This
  controls clock dividers, GPIO/LEDs and general  switch functionality. 

The playground image shows access to the different types of memory using the
SDCC compiler. Any support of Linux or e.g. Zephyr would require porting gcc.
There are FreeRTOS ports to 8051 processors using sdcc, however.

# Register differences between the RTL8372 and the RTL8373

`RTL837X_REG_SEC_COUNTER` (0x06F4) holds the seconds count: `handle_tick()`
reads its top byte, increments it and writes it back once per second. The
register only keeps a written value on the **RTL8373** - on an **RTL8372** it
always reads back `0x00000000` and never counts. `regget 06f4` tells the two
chip classes apart on the same board family, and the register's `DEBUG` test in
`rtlplayground.c` already notes that it depends on the SDS setup.

Anything that measured time from it therefore saw "0 seconds elapsed" on the
RTL8372:

- the reset button, whose press length used to be measured from it, reported
  every press as a short press and did nothing at all. It is now timed from the
  software tick counter (`handle_button()`), see
  [SWTG024AS](devices/SWTG024AS.md#why-the-button-used-to-do-nothing-at-all);
- the HTTP session timeout still compares against it, so sessions on those
  boards never expire. Deriving `read_reg_timer()` from the tick counter instead
  was tried and reverted: it broke session handling in practice.

