#include "machine_defs.h"

__code const struct machine machine = {
	/* RTL8372: MACHINE_MIN_PORT 3 / MAX 8 come from machine.h */
	.machine_name = "PB-2131",
	.model_name = "IG204 V1",
	.isRTL8373 = 0,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
	.phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
	.is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},
	// Left SFP port (physical 5, logical 8). Lamp on LED pad 9.
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	// Right SFP port (physical 6, logical 3). Lamp on LED pad 24.
	.sfp_port[1].pin_detect = GPIO50_I2C_SCL2_UART1_TX,
	.sfp_port[1].pin_los = GPIO51_I2C_SDA2_UART1_RX,
	.sfp_port[1].pin_tx_disable = GPIO_NA,
	.sfp_port[1].sds = 0,
	.sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO_NA,
	.high_leds = { .mux =  LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },
	/* Copper ports use SET0 with three pads each: slot 0 (amber, 2.5G),
	 * slot 1 (green, 1G/100M/10M) and a third pad the panel does not use.
	 * Both cages use SET1 and light at any speed. Verified on the unit:
	 * 2.5G = amber, and forcing the same port to 1G switches it to green.
	 */
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	.led_sets = {
			{
				/* RJ45 Amber LED: link at 2.5G, blinking while data
				 * flows. LEDS_ACT is required for the blink: without
				 * it the lamp stays solid during traffic, which is
				 * what the panel did. The other boards of this
				 * family (SWTG024AS, Hi-K0801WS) set it too, and
				 * the manual lists "闪烁:数据传输中" for the port
				 * lamp. */
				LEDS_2G5 | LEDS_LINK | LEDS_ACT,
				// RJ45 Green LED: 1G/100M/10M and blink on activity
				LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
				0,
				0,
			},
			{
				// SFP LED
				LEDS_10G | LEDS_5G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT,
				0, // unused
				0, // unused
				0
			},
		},
	.led_mux_custom = 1,
	/* Pads 0-7 are not wired to a lamp. They are pointed at 0x02, a signal
	 * this board never produces, instead of being left at 0x3f: a floating
	 * pad lights its lamp on boards whose lamp is pulled down.
	 */
	.led_mux = {
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0f, 0x20, 0x0d, 0x0e, 0x10, 0x11, 0x12, 0x14, 0x15, 0x16, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e, 0x0c, 0x21, 0x22, 0x23
	},
	/* The front-panel "VLAN ON/OFF" slide switch (left of the SYS LED) sits
	 * on GPIO36 and reads 1 when it is on, 0 when it is off (measured on the
	 * unit 2026-10-29). Note that PIN_MUX_1 bit 30 has to stay set for the
	 * pin level to reach the input register; gpio_input_setup() sets it. */
	.vlan_switch_pin = GPIO36_PWM_OUT,
	/* The manual: "SYS 慢闪 = the device works normally". */
	.sys_led_blink_idle = 1,
};
