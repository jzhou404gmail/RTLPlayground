#include "machine_defs.h"

__code const struct machine machine = {
	.machine_name = "F7005-2.5GPOE-V1.0",
	.model_name = "NPA AP-7024-2.5G",
	.isRTL8373 = 0,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
	.phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
	.is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},

	/* Left cage (logical 8, SDS1). Detect measured: GPIO38 goes low.
	 * GPIO34 also goes low on insertion; it is not mapped yet because the
	 * header has no GPIO34 alias. If the module is detected but never
	 * links, come back and try GPIO34 as TX_DISABLE/RX_LOS.
	 *
	 * The module EEPROM is NOT on the reference SDA4/SCL3 pins: a direct
	 * I2C read with the controller routed to SDA1/SCL1 returned the SFP
	 * identifier 0x03 and Encoding 0x06 / Rate 0x67 (10G). So SFP1 uses
	 * GPIO49=SDA1, GPIO48=SCL1. */
	.sfp_port[0].pin_detect = GPIO38,
	.sfp_port[0].pin_los = GPIO_NA,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO49_I2C_SDA1, GPIO48_I2C_SCL1 ),

	/* Right cage (logical 3, SDS0). Detect measured: GPIO37 goes low.
	 * Its EEPROM was measured on SDA0/SCL0 (GPIO47/46): a direct I2C read
	 * returned 0x03, Encoding 0x06 / Rate 0x67 for the same 10G module. */
	.sfp_port[1].pin_detect = GPIO37,
	.sfp_port[1].pin_los = GPIO_NA,
	.sfp_port[1].pin_tx_disable = GPIO_NA,
	.sfp_port[1].sds = 0,
	.sfp_port[1].i2c = I2CBUS( GPIO47_I2C_SDA0, GPIO46_I2C_SCL0 ),

	.reset_pin = GPIO_NA,

	/* The panel has a PWR lamp; whether the SoC drives it is not measured. */
	.high_leds = { .mux = LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },

	/* Copper ports use SET0, both cages SET1. */
	.port_led_set = {0, 0, 0, 1, 0, 0, 0, 0, 1},

	/* Front panel: 2.5G = green, 1000/100M = yellow. Slot 0 carries the
	 * 2.5G lamp and LEDS_ACT so that it blinks while data flows; the same
	 * bit was missing here as on the IG204 and left the lamp solid. */
	.led_sets = {
		{	/* SET0: copper */
			LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			0,
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			0
		},
		{	/* SET1: SFP, one lamp per cage, lit at any speed */
			LEDS_10G | LEDS_5G | LEDS_2G5 | LEDS_1G | LEDS_LINK | LEDS_ACT,
			0,
			0, 0
		},
	},

	.led_mux_custom = 1,
	.led_mux = {
		/* 0-2: SFP2 (right cage, logical 3) */
		0x0c, 0x0c, 0x02,
		/* 3-5: RJ45 1 (logical 4), green then orange */
		0x10, 0x12, 0x12,
		/* 6-8: RJ45 2 (logical 5), green then orange */
		0x14, 0x02, 0x16,
		/* 9-11: RJ45 3 green, 11 unused */
		0x18, 0x18, 0x02,
		/* 12-14: RJ45 3 orange */
		0x1a, 0x1a, 0x1a,
		/* 15-17: unused */
		0x02, 0x02, 0x02,
		/* 18-20: RJ45 4 green */
		0x1c, 0x1c, 0x1c,
		/* 21-23: RJ45 4 orange */
		0x1e, 0x1e, 0x1e,
		/* 24-27: SFP1 (left cage, logical 8) */
		0x20, 0x20, 0x20, 0x20
	},
};
