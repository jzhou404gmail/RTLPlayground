#include "machine_defs.h"

__code const struct machine machine = {
    .machine_name = "PCB-K0801W-V1.0",
    .model_name = "SFP-J08E-HG1",
    .isRTL8373 = 1,                     /* verified: boots with no 'INCORRECT MACHINE!' */
    .n_sfp = 1,
    .log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},   /* web UI lists the ports correctly */
    .phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},    /* a one-cable-per-jack pass would pin the order down */
    .is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},

    /* SFP cage, inherited from V2.0 (SDS1, GPIO30 = module detect,
     * GPIO37 = RX_LOS). Unverified on this revision: if 'sfp' reports an
     * empty cage with a module inserted, or the module EEPROM is
     * unreadable, measure these first. */
    .sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
    .sfp_port[0].pin_los = GPIO37,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 1,
    .sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
    .reset_pin = GPIO_NA,               /* no button and no footprint for one on this board */

    /* Not verified: inherited from V2.0, verify with 'leds' plus the
     * front-panel system LED */
    .high_leds = {
        .mux = LED_27 | LED_29,
        .enable = LED_28_SYS | LED_29
    },

    /* Not verified: inherited per-port LED set assignment (ports 1-8 -> set 0,
     * SFP -> set 1) */
    .port_led_set = {0, 0, 0, 0, 0, 0, 0, 0, 1},

    /* Not verified: inherited LED behaviour per speed/link. V2.0 lights
     * amber = 2.5G, green = 1G/100M/10M with activity. */
    .led_sets = {
        {   /* Set 0: RJ45 copper ports */
            LEDS_2G5 | LEDS_LINK | LEDS_ACT, /* Amber */
            0,
            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT, /* Green */
            0
        },
        {   /* Set 1: SFP port, single green LED for all valid speeds */
            LEDS_10G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
            0,
            0,
            0
        },
    },
};
