#include "machine_defs.h"

__code const struct machine machine = {
    .machine_name = "PCB-SWTG024AS-V1.0",
    .model_name = "SKS3200M-4GPY2XF",
    .isRTL8373 = 0,
    /*
     * The stock firmware of this unit reported MAC 1C:2A:A3:23:E4:5E, so the
     * board keeps its factory MAC in flash the way the ZX-SWTGW215AS boards do.
     * Only this target reads it - for the V2.0 boards of the family that area
     * of the flash is unverified.
     */
    .mac_flash_offset = 0x1FC000,
    /* Jack 5 is the cage on 10GMAC 8 / SerDes 1 and jack 6 the one on 10GMAC 3
     * / SerDes 0 (see the port overview in doc/devices/SWTG024AS.md), so the
     * two SFP jacks cross over in the logical numbering. */
    .log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
    .phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
    /*
     * Both cages are SFP on this board: jack 5 (J4, SFP1) is logical port 8,
     * which is where the sibling boards put their single cage - detect GPIO30,
     * RX-LOS GPIO37, I2C SDA4/SCL3, SerDes 1 - and jack 6 (J2, SFP2) is logical
     * port 3 - detect GPIO50, RX-LOS GPIO51, I2C SDA3/SCL3, SerDes 0. The unit
     * shipped with one slot that watched SFP1's pins but belonged to logical
     * port 3, so a module in jack 5 was reported - and lit - as jack 6 while a
     * module in jack 6 was not seen at all (found 2026-10-13).
     */
    .n_sfp = 2,
    .is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},
    .sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
    .sfp_port[0].pin_los = GPIO37,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 1,
    .sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
    .sfp_port[1].pin_detect = GPIO50_I2C_SCL2_UART1_TX,
    .sfp_port[1].pin_los = GPIO51_I2C_SDA2_UART1_RX,
    .sfp_port[1].pin_tx_disable = GPIO_NA,
    .sfp_port[1].sds = 0,
    .sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),

    /*
     * K1, the reset button of this PCB, is on GPIO54 (doc/devices/SWTG024AS.md,
     * "Reset circuit"). The pin is shared with SFP2's TX-DISABLE through R105 /
     * R85, and both TX-disable pins are GPIO_NA on this board, so nothing else
     * drives it. Only the XikeStor target gets it: on the V2.0 boards of this
     * family R85 and K1 may not be populated, and a pin that then looks
     * permanently pressed would make the boot-time button handler restore the
     * default configuration on every power-up.
     */
    .reset_pin = GPIO54_ACL_BIT2_EN,
    .high_leds = { .mux =  LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },
    /*
     * Measured on the unit (2026-10-13, by driving one pad at a time through
     * the LED controller and watching the front panel, see the "LEDs" section
     * of doc/devices/SWTG024AS.md). The lamps are NOT on the pads the V2.0
     * table assumes: green/amber sit on pad 12/14 (port 1), 15/17 (port 2),
     * 18/20 (port 3) and 21/23 (port 4), and the two SFP lamps on pad 24
     * (SFP1) and pad 9 (SFP2). The mux index is the pad, the value is the LED
     * signal driving it, id = ASIC port * 4 + slot, with slot 0 =
     * 2G5|LINK|ACT (green) and slot 1 = 1G|100M|10M|LINK|ACT (amber). The
     * value 4 is a port this board does not have, so it is never true: the
     * pad is then driven to its inactive level, while leaving it unset (0x3f)
     * lets it float - and a floating lamp is lit on every pad that is pulled
     * down, which is what made the ambers glow with no link.
     */
    .port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
    .led_sets = {
                    {
                            LEDS_2G5 | LEDS_LINK | LEDS_ACT,
                            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
                            LEDS_DUPLEX,
                            LEDS_2G5 | LEDS_LINK | LEDS_ACT
                    },
                    {
                            /* the SFP cages have one lamp each: light it at
                             * any speed, not only at 2.5G/1G/100M */
                            LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_10G | LEDS_LINK | LEDS_ACT,
                            LEDS_10G | LEDS_LINK | LEDS_ACT,
                            LEDS_2G5 | LEDS_LINK,
                            LEDS_COL | LEDS_DUPLEX
                    },
     },
    .led_mux_custom = 1,
    .led_mux = {
                            0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x0c,0x04,0x04,0x10,0x04,0x11,0x14,0x04,0x15,0x18,0x04,0x19,0x1c,0x04,0x1d,0x20,0x04,0x04,0x04
            },
        .id_layout = 1,
};
