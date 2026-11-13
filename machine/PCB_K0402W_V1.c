#include "machine_defs.h"

__code const struct machine machine = {
    .machine_name = "PCB-K0402W-V1.0",
    .model_name = "Hi-K0402WS",
    .isRTL8373 = 0,                     /* verified: 'Detecting CPU: RTL8372' */
    .n_sfp = 2,
    .log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},   /* verified: front RJ45 1-4 -> physical 1-4 */
    .phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
    .is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},

    /* Left SFP (logical 8, SDS1). Verified with a module in the cage. */
    .sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
    .sfp_port[0].pin_los = GPIO37,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 1,
    .sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

    /* Right SFP (logical 3, SDS0). Verified with a module in the cage. */
    .sfp_port[1].pin_detect = GPIO50_I2C_SCL2_UART1_TX,
    .sfp_port[1].pin_los = GPIO51_I2C_SDA2_UART1_RX,
    .sfp_port[1].pin_tx_disable = GPIO_NA,
    .sfp_port[1].sds = 0,
    .sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),

    /* No reset button and no unpopulated footprint for one on this board. */
    .reset_pin = GPIO_NA,

    /* Not measurable here: the front-panel power LED is hard-wired, so this
     * value drives nothing. Kept at the V2.0 setting. */
    .high_leds = { .mux = LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },

    /* Verified: copper ports use SET0, both SFP ports use SET1. */
    .port_led_set = {0, 0, 0, 1, 0, 0, 0, 0, 1},

    /* Verified on the board: copper amber = 2.5G, green = 1G/100M/10M with
     * activity; the SFP LED lights while a module is linked. */
    .led_sets = {
        {   /* SET0: copper */
            LEDS_2G5 | LEDS_LINK | LEDS_ACT,
            0,
            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
            0
        },
        {   /* SET1: SFP — all speeds link/act */
            LEDS_10G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
            0,
            0, 0
        },
    },

    /* The V2.0 pad->LED map, confirmed on this board by the set-swap
     * experiment in doc/devices/PCB-K0402W-V1.0.md. */
    .led_mux_custom = 1,
    .led_mux = {
        0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,  /* GPIO0-7: unused */
        0x0f, 0x0c, 0x0d, 0x0e, 0x10, 0x11, 0x12,          /* GPIO8-14 */
        0x14, 0x15, 0x16, 0x18, 0x19, 0x1a,                 /* GPIO15-20 */
        0x1c, 0x1d, 0x1e, 0x20, 0x21, 0x22, 0x23            /* GPIO21-27 */
    },
};
