#include "machine_defs.h"

/*
 * PCB-K0402W-V3.0 / DIP-K0402WS-V3.0 as sold by MIMC as D4S2TX
 * ("4 x 2.5G + 2 x SFP+/10G").
 *
 * The front-panel "Vlan S/V" slide switch is NOT a VLAN switch: it is the
 * flash selector. Measured on the unit 2026-11-17: S selects U1 (front,
 * 2 MiB) and V selects U67 (back, 2 MiB); both are the same
 * BY25Q16ES-class part and the switch drives their HOLD lines, so flipping
 * it reboots into the other chip. There is therefore no vlan_switch_pin on
 * this board and the panel-VLAN feature stays off.
 *
 * Family: this board is the same PCB and panel as the upstream
 * PCB_K0402W_V3_0 target; its LED configuration (led_mux / led_sets /
 * high_leds) and the pad-6 init hook are taken from that target unchanged.
 * Verified on the unit 2026-11-17 by flashing the upstream build: every panel
 * lamp (L1-L4, R1-R4 and both SFP cages) showed the correct colour and state.
 *
 * The SFP detect pins follow the V3.0 revision: GPIO30 is TP131 (reset mod,
 * documented upstream), so it is no longer the left cage's ModAbs as on
 * V1.0/V2.0. Run `sfp` if a cage misbehaves.
 * See doc/devices/PCB-K0402W-V3.0.md.
 */
__code const struct machine machine = {
    /* The System page shows a single "Model" row with this name in the
     * hardware slot (after uptime); machine_name is still the image's
     * identity used by the CLI and the web updater's target check. */
    .machine_name = "MIMC D4S2TX",
    .model_name = "MIMC D4S2TX",
    .isRTL8373 = 0,
    .n_sfp = 2,
    .log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
    .phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
    .is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},

    /* Left SFP (logical 8, SDS1). V3.0: ModAbs on GPIO38, no LOS pin. */
    .sfp_port[0].pin_detect = GPIO38,
    .sfp_port[0].pin_los = GPIO_NA,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 1,
    .sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

    /* Right SFP (logical 3, SDS0). V3.0: ModAbs on GPIO37, no LOS pin. */
    .sfp_port[1].pin_detect = GPIO37,
    .sfp_port[1].pin_los = GPIO_NA,
    .sfp_port[1].pin_tx_disable = GPIO_NA,
    .sfp_port[1].sds = 0,
    .sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),

    /* No reset button on this unit. The upstream V3.0 definition uses GPIO30
     * for the optional TP131 reset mod; this unit has no button fitted, so
     * leave the pin alone (a floating pad must not trigger a factory reset). */
    .reset_pin = GPIO_NA,

    /* LED configuration taken unchanged from the upstream
     * PCB_K0402W_V3_0 definition (see machine/pcb/PCB_K0402W_V3_0.c) after
     * flashing that target on this unit on 2026-11-17: every panel lamp -
     * L1-L4, R1-R4 and both SFP cages - showed the correct colour and state.
     * The earlier V1.0-derived map in this file was wrong. */
    .high_leds = { .mux = LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },

    /* Copper ports use LED SET0, both cages SET1. */
    .port_led_set = {0, 0, 0, 1, 0, 0, 0, 0, 1},

    .led_sets = {
        {   /* SET0: copper */
            LEDS_2G5 | LEDS_LINK | LEDS_10M | LEDS_ACT,
            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
            LEDS_1G | LEDS_LINK,
            0
        },
        {   /* SET1: both SFP cages */
            LEDS_10G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK,
            LEDS_10G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_ACT,
            0,
            0
        },
    },

    .led_mux_custom = 1,
    .led_mux = {
        0x00, 0x01, 0x04, 0x05, 0x08, 0x09, 0x0c, 0x3f,  /* 0-7 */
        0x0d, 0x10, 0x11, 0x0e, 0x14, 0x11, 0x12, 0x15,  /* 8-15 */
        0x15, 0x16, 0x18, 0x19, 0x1a, 0x19, 0x1d, 0x1e,  /* 16-23 */
        0x1c, 0x1d, 0x20, 0x21                          /* 24-27 */
    },

    /* Two populated 2 MiB flashes (U1 front, U67 back) selected by the front
     * S/V switch via their HOLD lines; the switch is hardware only. The
     * firmware reads the selected chip in single-bit mode, which both parts
     * handle. */
    .flash_sio = 1,
    .id_layout = 1,
};
