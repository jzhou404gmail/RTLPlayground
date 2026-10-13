#include "machine.h"
#include "rtl837x_pins.h"
#include "rtl837x_leds.h"
#include "rtl837x_sfr.h"
#include "rtl837x_regs.h"
#include "rtl837x_common.h"

#define I2CBUS_INVALID (7)
// This is a MACRO to check if I2C_BUS_FROM_SDA_PIN or I2C_BUS_FROM_SCL_PIN don't return I2CBUS_INVALID.
// When I2CBUS_INVALID is returned, compiler stops with "machine.c:<line>: error 176: sizeof applied to an incomplete type"
#define I2CBUS_VALID(bus) (0 * sizeof(char[(bus) != I2CBUS_INVALID ? 1 : 0]))
#define I2CBUS(sda, scl) \
	((uint8_t)((I2C_BUS_FROM_SCL_PIN(scl) << RTL837X_REG_I2C_SCL_SHIFT) | \
		   (I2C_BUS_FROM_SDA_PIN(sda) << RTL837X_REG_I2C_SDA_SHIFT) | \
		   I2CBUS_VALID(I2C_BUS_FROM_SCL_PIN(scl)) | \
		   I2CBUS_VALID(I2C_BUS_FROM_SDA_PIN(sda))))

#define I2C_BUS_FROM_SDA_PIN(sda_pin) \
	((sda_pin) == GPIO47_I2C_SDA0 ? 0 : \
	 (sda_pin) == GPIO49_I2C_SDA1 ? 1 : \
	 (sda_pin) == GPIO51_I2C_SDA2_UART1_RX ? 2 : \
	 (sda_pin) == GPIO41_I2C_SDA3_MDIO1 ? 3 : \
	 (sda_pin) == GPIO39_I2C_SDA4 ? 4 : I2CBUS_INVALID)

#define I2C_BUS_FROM_SCL_PIN(scl_pin) \
	((scl_pin) == GPIO46_I2C_SCL0 ? 0 : \
	 (scl_pin) == GPIO48_I2C_SCL1 ? 1 : \
	 (scl_pin) == GPIO50_I2C_SCL2_UART1_TX ? 2 : \
	 (scl_pin) == GPIO40_I2C_SCL3_MDC1 ? 3 : I2CBUS_INVALID)

#if defined(MACHINE_KP_9000_6XHML_X2) || \
	defined(MACHINE_KP_9000_6XH_X2_V1_1) || \
	defined(MACHINE_KP_9000_6XHML_X2_V1_1) || \
	defined(MACHINE_KP_9000_6XH_X2_V1_2) || \
	defined(MACHINE_KP_9000_6XHML_X2_V1_2)
__code const struct machine machine = {
#if defined(MACHINE_KP_9000_6XH_X2_V1_1)
	.machine_name = "keepLink KP-9000-6XH V1.1",
#elif defined(MACHINE_KP_9000_6XHML_X2_V1_1)
	.machine_name = "keepLink KP-9000-6XHML V1.1",
#elif defined(MACHINE_KP_9000_6XH_X2_V1_2)
	.machine_name = "keepLink KP-9000-6XH V1.2",
#elif defined(MACHINE_KP_9000_6XHML_X2_V1_2)
	.machine_name = "keepLink KP-9000-6XHML V1.2",
#else
	.machine_name = "keepLink KP-9000-6XHML-X2",
#endif
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
	.phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
	.is_sfp = {0, 0, 0, 1, 0, 0, 0, 0, 2},
	// Left SFP port (5)
	.sfp_port[0].pin_detect = GPIO50_I2C_SCL2_UART1_TX,
	.sfp_port[0].pin_los = GPIO10_LED10,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 0,
	.sfp_port[0].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),
	// Right SFP port (6)
	.sfp_port[1].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[1].pin_los = GPIO37,
	.sfp_port[1].pin_tx_disable = GPIO_NA,
	.sfp_port[1].sds = 1,
	.sfp_port[1].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO54_ACL_BIT2_EN,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 0},
	/* Conditions for LED on:
	 * dual led orange: ledset_0 & ledset_2
	 * dual led green: ledset_2 & !ledset_0
	 * single right led green: ledset_0 & !ledset_1
	*/
	.led_sets = { { LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
			LEDS_2G5 | LEDS_LINK | LEDS_10G,
			LEDS_1G | LEDS_LINK,
			0 },
		    },
};

#elif defined MACHINE_KP_9000_6XH_X
__code const struct machine machine = {
	.machine_name = "keepLink KP-9000-6XH-X",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
	.phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO_NA,
	/* Conditions for LED on:
	 * dual led orange: ledset_0 & ledset_2
	 * dual led green: ledset_2 & !ledset_0
	 * single right led green: ledset_0 & !ledset_1
	*/
	.led_sets = { { LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
			LEDS_2G5 | LEDS_LINK | LEDS_10G,
			LEDS_1G | LEDS_LINK,
			0 },
		    },
};

#elif defined(MACHINE_KP_9000_6XH_X2) || defined(MACHINE_KP_9000_6XH_X2_V2_1) || defined(MACHINE_KP_9000_6XHML_X2_V2_1)
__code const struct machine machine = {
#if defined(MACHINE_KP_9000_6XHML_X2_V2_1)
	.machine_name = "keepLink KP-9000-6XHML V2.1",
#elif defined(MACHINE_KP_9000_6XH_X2_V2_1)
	.machine_name = "keepLink KP-9000-6XH-X2 V2.1",
#else
	.machine_name = "keepLink KP-9000-6XH-X2",
#endif
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
	.phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
	.is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},

	// Left SFP port
	.sfp_port[0].pin_detect = GPIO38,
	.sfp_port[0].pin_los = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

	// Right SFP port
	.sfp_port[1].pin_detect = GPIO37,
	.sfp_port[1].pin_los = GPIO_NA,
	.sfp_port[1].sds = 0,
	.sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),

	.reset_pin = GPIO48_I2C_SCL1,   // Button-Switch is unpopulated on PCB, but can be added manually (hole in case is already there)
	.high_leds = { .mux =  LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	.led_sets = {
			{
				LEDS_2G5 | LEDS_LINK,												// Left LED (Amber)
				LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,   // Right LED (Green)
				0,
				0
			},
			{
				LEDS_10G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
				0,
				0,
				0
			},
	 },
	.led_mux_custom = 1,
	.led_mux = {
				0x00,0x01,0x04,0x05,0x08,0x09,0x0c,0x3f,0x0d,0x10,
				0x11,0x0e,0x14,0x11,0x12,0x15,0x15,0x16,0x18,0x19,
				0x1a,0x19,0x1d,0x1e,0x1c,0x1d,0x20,0x21
		},
	};

#elif defined MACHINE_KP_9000_9XH_X_EU
__code const struct machine machine = {
	.machine_name = "keepLink KP-9000-9XH-X-EU",
	.isRTL8373 = 1,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO_NA,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.led_sets = { { LEDS_2G5 | LEDS_TWO_PAIR_1G | LEDS_1G | LEDS_500M | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G | LEDS_TWO_PAIR_5G | LEDS_5G | LEDS_TWO_PAIR_2G5,
			LEDS_2G5 | LEDS_LINK,
			LEDS_1G | LEDS_LINK, 
			LEDS_2G5 | LEDS_LINK | LEDS_ACT },
		    },
};

#elif defined MACHINE_KP_9000_9XHML_X_V2_2
__code const struct machine machine = {
	.machine_name = "keepLink KP-9000-9XHML-X V2.2",
	.isRTL8373 = 1,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = { 1, 2, 3, 4, 5, 6, 7, 8, 9 },
	.phys_to_log_port = { 0, 1, 2, 3, 4, 5, 6, 7, 8 },
	.is_sfp = { 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO54_ACL_BIT2_EN,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	.led_sets = {
		{   /* Set 0 for RJ45 connectors */
			/* LED0: Right Green */
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			/* LED1: Left Orange */
			LEDS_2G5 | LEDS_LINK,
			/* LED2: Left Green */
			LEDS_1G | LEDS_LINK,
			/* LED3: None */
			0,
		}, { /* Set 1 for SFP port */
			/* LED0: Single Green "9" LED */
			LEDS_2G5 | LEDS_TWO_PAIR_1G | LEDS_1G | LEDS_500M | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G | LEDS_TWO_PAIR_5G | LEDS_5G | LEDS_TWO_PAIR_2G5,
			/* LED1: D23 LED on PCB */
			LEDS_1G | LEDS_LINK,
			/* LED2: D22 LED on PCB */
			LEDS_2G5 | LEDS_LINK,
			/* LED3: Unused */
			LEDS_COL | LEDS_DUPLEX,
		}, { /* Set 2: Unused, but set to same things as stock firmware for consistency */
			LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			LEDS_1G | LEDS_LINK | LEDS_ACT,
			LEDS_2G5 | LEDS_LINK | LEDS_ACT | LEDS_5G,
			LEDS_10G | LEDS_ACT | LEDS_LINK,
		}, { /* Set 3: Unused, but set to same things as stock firmware for consistency */
			LEDS_TX,
			LEDS_RX,
			LEDS_2G5 | LEDS_TWO_PAIR_1G | LEDS_1G | LEDS_500M | LEDS_100M | LEDS_10M | LEDS_ACT | LEDS_10G | LEDS_TWO_PAIR_5G | LEDS_5G | LEDS_TWO_PAIR_2G5,
			LEDS_2G5 | LEDS_TWO_PAIR_1G | LEDS_1G | LEDS_500M | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_10G | LEDS_TWO_PAIR_5G | LEDS_5G | LEDS_TWO_PAIR_2G5,
		},
	},
};

#elif defined(MACHINE_KP_9000_9XHML_X_V3_1) || defined(MACHINE_KP_9000_9XHML_X_V3_2)
// 2M-PCB23-V3.2 is wired like V3.1 (SFP detection, LEDs, factory MAC location)
__code const struct machine machine = {
#if defined(MACHINE_KP_9000_9XHML_X_V3_2)
	.machine_name = "keepLink KP-9000-9XHML-X V3.2",
#else
	.machine_name = "keepLink KP-9000-9XHML-X V3.1",
#endif
	.isRTL8373 = 1,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO38,
	.sfp_port[0].pin_los = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO48_I2C_SCL1,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	.led_sets = {
		{   /* RJ45: First LED, yellow, second LED: green */
			LEDS_2G5 | LEDS_LINK,
            LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			0,
			0,
		}, { /* SFP PORT, SINGLE GREEN LED */
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
			0,
			0,
			0,
		}},
	.led_mux_custom = 1,
	.led_mux = {0x00, 0x01, 0x04, 0x05, 0x08, 0x09, 0x0c, 0x09, 0x0d, 0x10,
				0x11, 0x0e, 0x14, 0x11, 0x12, 0x15, 0x15, 0x16, 0x18, 0x19,
				0x1a, 0x19, 0x1d, 0x1e, 0x1c, 0x1d, 0x20, 0x21},
	.mac_flash_offset = 0x1FC000,
};

#elif defined MACHINE_SWGT024_V2_0_MANAGED
__code const struct machine machine = {
	.machine_name = "SWGT024 V2.0 Managed",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
	.phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
	.is_sfp= {0, 0, 0, 2, 0, 0, 0, 0, 1},
	// Left SFP port (J4)
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO_NA,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ), /* GPIO 39 */
	// Right SFP port (J2)
	.sfp_port[1].pin_detect = GPIO50_I2C_SCL2_UART1_TX,
	.sfp_port[1].pin_los = GPIO_NA,
	.sfp_port[1].pin_tx_disable = GPIO_NA,
	.sfp_port[1].sds = 0,
	.sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ), /* GPIO 40 */
	.reset_pin = GPIO54_ACL_BIT2_EN,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	.led_sets = {
		{ /* RJ45: Green LED */
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
			0,
			/* Amber LED */
			LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			0
		}, { /* SFP PORT: SINGLE GREEN LED */
			LEDS_10M | LEDS_100M | LEDS_1G | LEDS_2G5 | LEDS_10G | LEDS_LINK | LEDS_ACT,
			0,
			0,
			0,
		},
	},
};

#elif defined MACHINE_SWGT024_V2_0_UNMANAGED
__code const struct machine machine = {
	.machine_name = "SWGT024 V2.0 Unmanaged",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
	.phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
	.is_sfp= {0, 0, 0, 2, 0, 0, 0, 0, 1},
	// Left SFP port (J4)
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ), /* GPIO 39 */
	// Right SFP port (J2)
	.sfp_port[1].pin_detect = GPIO50_I2C_SCL2_UART1_TX,
	.sfp_port[1].pin_los = GPIO51_I2C_SDA2_UART1_RX,
	.sfp_port[1].pin_tx_disable = GPIO_NA,
	.sfp_port[1].sds = 0,
	.sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ), /* GPIO 40 */
	.reset_pin = GPIO_NA,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	.led_sets = {
		{ /* RJ45: Green LED */
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
			0,
			/* Amber LED */
			LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			0
		}, { /* SFP PORT: SINGLE GREEN LED */
			LEDS_10M | LEDS_100M | LEDS_1G | LEDS_2G5 | LEDS_10G | LEDS_LINK | LEDS_ACT,
			0,
			0,
			0,
		},
	},
};

#elif defined MACHINE_SWTG018AS_A_V_2_0
__code const struct machine machine = {
	.machine_name = "SWTG018AS-A V2.0",
	.isRTL8373 = 1,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO38,
	.sfp_port[0].pin_los = GPIO_NA,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO48_I2C_SCL1,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	.led_sets = {
		{   /* RJ45: First LED, yellow, second LED: green */
			LEDS_2G5 | LEDS_LINK,
            LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			0,
			0,
		}, { /* SFP PORT, SINGLE GREEN LED */
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
			0,
			0,
			0,
		}},
	.led_mux_custom = 1,
	.led_mux = { 0x00, 0x01, 0x04, 0x05, 0x08, // 65e0
		     0x09, 0x0c, 0x09, 0x0d, 0x10, // 65e4
		     0x11, 0x0e, 0x14, 0x11, 0x12, // 65e8
		     0x15, 0x15, 0x16, 0x18, 0x19, // 65ec
		     0x1a, 0x19, 0x1d, 0x1e, 0x1c, // 65f0
		     0x1d, 0x20, 0x21 },
};

#elif defined MACHINE_HG0402XG_V1_1
__code const struct machine machine = {
	.machine_name = "HG0402XG V1.1",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
	.phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
	.is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = 50,
	.sfp_port[0].pin_los = 10,
	.sfp_port[0].pin_tx_disable = 0xFF,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),
	.sfp_port[1].pin_detect = 30,
	.sfp_port[1].pin_los = 51,
	.sfp_port[1].pin_tx_disable = 0xFF,
	.sfp_port[1].sds = 0,
	.sfp_port[1].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO_NA,
	.high_leds = { .mux = LED_27 , .enable = LED_27 | LED_29 },
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	/* The Ethernet ports have 1 amber LED (left) and 1 green LED (right)
	 * The SFP ports have also 1 amber LED and 1 green LED
	 * Ethernet ports use LED-set 0, SFP ports use LED-set 1
	 */
	.led_sets = { { LEDS_10M | LEDS_LINK | LEDS_ACT,
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			0 },
			{ LEDS_100M | LEDS_10M | LEDS_LINK,
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
			LEDS_10G | LEDS_LINK,
			0 },
		    },
};

#elif defined(MACHINE_SWTGW218AS) || defined(MACHINE_SKS3200M_8GPY1XF)
/*
 * SWTG118AS-V2.x - 8 x 2.5GBASE-T + 1 x SFP+ (RTL8373 + RTL8224). Sold as
 * Mokerlink/Horaco/Sodola ZX-SWTGW218AS on SWTG118AS-V2.0-16029 and as XikeStor
 * SKS3200M-8GPY1XF on SWTG118AS-V2.1-17462 (the owner's unit states that model in
 * its web UI and on its label; an earlier revision of the device list called it
 * SKS3200-8E1X, which no unit seen so far carries). Both revisions take the same
 * wiring, so they share this block and differ in how they name the board: the
 * XikeStor target puts the PCB revision in .machine_name and the retail model in
 * .model_name, which the web UI shows as 硬件 (hardware) and 型号 (model).
 */
__code const struct machine machine = {
#if defined(MACHINE_SKS3200M_8GPY1XF)
	.machine_name = "PCB-SWTG118AS-V2.1",
	.model_name = "SKS3200M-8GPY1XF",
#else
	.machine_name = "SWTGW218AS 8+1 Managed Switch",
#endif
	.isRTL8373 = 1,
	.mac_flash_offset = 0x1FC000,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO54_ACL_BIT2_EN,
	.high_leds = { .mux = LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	.led_sets = { { LEDS_2G5 | LEDS_LINK | LEDS_ACT, // Green LED (right)
					0, // unused
					LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT, // Amber LED (left)
					0
				  }, // unused
				  { LEDS_10G | LEDS_5G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT, // SFP LED
					0, // unused
					0, // unused
					0
				  }, // unused		    	},
				},
};

#elif defined MACHINE_PCB_SWTG018AS_V2_1_0 // Sold as Sodola SL902 / Horaco "SWTGW218AS"; the SWTGW218AS label also covers other PCBs with different SFP and LED wiring (see MACHINE_SWTGW218AS)

__code const struct machine machine = {
	.machine_name = "SWTGW218AS (SWTG018AS-V2.1.0)",
	.isRTL8373 = 1,
	.mac_flash_offset = 0x1FC000,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO38,   // pulled low on module insert
	.sfp_port[0].pin_los = GPIO_NA,     // no LOS pin wired
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO54_ACL_BIT2_EN,
	.high_leds = { .mux = LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	// LED wiring matches the SWTG018AS-A V2.0 (same PCB family)
	.led_sets = {
		{   /* RJ45: First LED, yellow, second LED: green */
			LEDS_2G5 | LEDS_LINK,
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			0,
			0,
		}, { /* SFP set (superseded by the raw register override in machine_custom_init) */
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
			0,
			0,
			0,
		}},
	.led_mux_custom = 1,
	.led_mux = { 0x00, 0x01, 0x04, 0x05, 0x08, // 65e0
		     0x09, 0x0c, 0x09, 0x0d, 0x10, // 65e4
		     0x11, 0x0e, 0x14, 0x11, 0x12, // 65e8
		     0x15, 0x15, 0x16, 0x18, 0x19, // 65ec
		     0x1a, 0x19, 0x1d, 0x1e, 0x1c, // 65f0
		     0x1d, 0x20, 0x21 },
};

#elif defined(MACHINE_LIANGUO_ZX_SWTGW215AS) || defined(MACHINE_SKS3200M_5GPY1XF)
/*
 * PCB-SWTG115AS-V2.x - 5 x 2.5GBASE-T + 1 x SFP+, RTL8372, one RTL8221B for the
 * 5th RJ45. Sold under several brands: MACHINE_LIANGUO_ZX_SWTGW215AS covers the
 * units labelled ZX-SWTGW215AS (PCB-SWTG115AS-V2.0, and V2.1 which ships
 * unbranded), MACHINE_SKS3200M_5GPY1XF the XikeStor SKS3200M-5GPY1XF ("兮克",
 * 5 x 2.5G + 1 x 10G SFP+), whose board is silkscreened PCB-SWTG115AS-V2.1-17443
 * (2023-11-04) - the same PCB revision and the same LED wiring. Both targets
 * share every hardware parameter below. They differ in how they name the board:
 * MACHINE_SKS3200M_5GPY1XF puts the board revision in .machine_name and the
 * retail model in .model_name, so the web UI and the boot log show 型号 (model)
 * and 硬件 (hardware) as two separate lines, while MACHINE_LIANGUO_ZX_SWTGW215AS
 * keeps brand + model in .machine_name, which is what its users see today.
 *
 * Verified on the SKS3200M-5GPY1XF (2026-10-12, read over the web interface):
 * `regget 0004` = 0x83720000 (RTL8372, the 4-PHY class), only switch ports 3..8
 * carry a PHY, the 4th RJ45 jack comes up as switch port 7, and an inserted SFP
 * module is detected on GPIO30 with its EEPROM readable over I2C SDA4/SCL3 -
 * all of which matches this block. See doc/devices/SKS3200M-5GPY1XF.md.
 */
__code const struct machine machine = {
#if defined(MACHINE_SKS3200M_5GPY1XF)
	.machine_name = "PCB-SWTG115AS-V2.1",
	.model_name = "SKS3200M-5GPY1XF",
#else
	.machine_name = "Lianguo ZX-SWTGW215AS",
#endif
	.isRTL8373 = 0,
	.mac_flash_offset = 0x1FC000,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
	.phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO54_ACL_BIT2_EN,
	.high_leds = { .mux = LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	.led_sets = { { LEDS_2G5 | LEDS_LINK | LEDS_ACT, // Green LED (right)
					0, // unused
					LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT, // Amber LED (left)
					0
				  }, // unused
				  { LEDS_10G | LEDS_5G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT, // SFP LED
					0, // unused
					0, // unused
					0
				  }, // unused		    	},
				},
	.led_mux_custom = 0,
};

#elif defined MACHINE_DEFAULT_8C_1SFP
__code const struct machine machine = {
	.machine_name = "8+1 SFP Port Switch",
	.isRTL8373 = 1,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO_NA,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.led_sets = { { LEDS_2G5 | LEDS_TWO_PAIR_1G | LEDS_1G | LEDS_500M | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G | LEDS_TWO_PAIR_5G | LEDS_5G | LEDS_TWO_PAIR_2G5,
			LEDS_2G5 | LEDS_LINK,
			LEDS_1G | LEDS_LINK,
			LEDS_2G5 | LEDS_LINK | LEDS_ACT },
		    },
};

#elif defined MACHINE_TRENDNET_TEG_S562
__code const struct machine machine = {
	.machine_name = "Trendnet TEG-S562",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
	.phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
	.is_sfp = {0, 0, 0, 1, 0, 0, 0, 0, 2},
	.sfp_port[0].pin_detect = GPIO38,
	.sfp_port[0].pin_los = GPIO50_I2C_SCL2_UART1_TX,
	.sfp_port[0].pin_tx_disable = GPIO54_ACL_BIT2_EN,
	.sfp_port[0].sds = 0,
	.sfp_port[0].i2c = I2CBUS( GPIO47_I2C_SDA0, GPIO46_I2C_SCL0 ),
	.sfp_port[1].pin_detect = GPIO36_PWM_OUT,
	.sfp_port[1].pin_los = GPIO37,
	.sfp_port[1].pin_tx_disable = GPIO51_I2C_SDA2_UART1_RX,
	.sfp_port[1].sds = 1,
	.sfp_port[1].i2c = I2CBUS( GPIO49_I2C_SDA1, GPIO48_I2C_SCL1 ),
	.reset_pin = GPIO_NA,
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	.led_sets = {
		{
			0, // Unused
			LEDS_2G5 | LEDS_LINK | LEDS_ACT, // Green
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT, // Amber
			0 // Unused
		},
		{
			0, // Unused
			0, // Unused
			LEDS_10G | LEDS_1G | LEDS_LINK | LEDS_ACT, // Green
			0, // Unused
		},
	},

};

#elif defined(MACHINE_PCB_K0402WS_V3) || defined(MACHINE_HI_K0402WS)  // Sold as a variety of devices, see doc/
__code const struct machine machine = {
	.machine_name = "PCB-K0402WS-V3.0",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
	.phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
	.is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},
	
	// Left SFP port
	.sfp_port[0].pin_detect = GPIO38, 
	.sfp_port[0].pin_los = GPIO_NA, 
	.sfp_port[0].sds = 1, 
	.sfp_port[0].i2c =  I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

	// Right SFP port
	.sfp_port[1].pin_detect = GPIO37,
	.sfp_port[1].pin_los = GPIO_NA, 
	.sfp_port[1].sds = 0, 
	.sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),

	.reset_pin = GPIO_NA,
	.high_leds = { .mux =  LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	.led_sets = { 
			{
				LEDS_2G5 | LEDS_LINK | LEDS_10M | LEDS_ACT,
				LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
				LEDS_1G | LEDS_LINK, 
				0 
			},
			{  
				LEDS_10G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK,
				LEDS_10G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_ACT,
				0,
				0
			},
	 },
	.led_mux_custom = 1,
	.led_mux = {
				0x00,0x01,0x04,0x05,0x08,0x09,0x0c,0x3f,0x0d,0x10,0x11,0x0e,0x14,0x11,0x12,0x15,0x15,0x16,0x18,0x19,0x1a,0x19,0x1d,0x1e,0x1c,0x1d,0x20,0x21
		},
	};

#elif defined MACHINE_K0501W_V2_0
__code const struct machine machine = {
	.machine_name = "K0501W V2.0",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
	.phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c =  I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

	.reset_pin = GPIO_NA,
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	.led_sets = {
		{
			LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			0,
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			0
		},
		{
			LEDS_10G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			0,
			0,
			0
		},
	 },
};

#elif defined MACHINE_ZX310S_4T2XH
__code const struct machine machine = {
	.machine_name = "ZX310S-4T2XH",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 1,
	.n_10g = 1,
	.log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
	.phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO38,
	.sfp_port[0].pin_los = GPIO_NA,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO48_I2C_SCL1,
	.high_leds = { .mux =  LED_28_SYS, .enable = LED_27 | LED_28_SYS | LED_29 },
	.led_mux_custom = 1,
	.led_mux = { 0x00, 0x01, 0x04, 0x05, 0x08, // 65e0
		     0x09, 0x0c, 0x3f, 0x0d, 0x10, // 65e4
		     0x11, 0x0e, 0x14, 0x11, 0x12, // 65e8
		     0x15, 0x15, 0x16, 0x18, 0x19, // 65ec
		     0x1a, 0x19, 0x1d, 0x1e, 0x1c, // 65f0
		     0x1d, 0x20, 0x21 },
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	/* Ports 1-4: Orange: 2.5GBit, Green: 10/100/1000MBit
	 * Port 5: Blue: 10GBit, Green: 10Mbit-5GBit
	 * SFP-port: Blue: 10GBit, Green 100MBit-5GBit
	 */
	.led_sets = { { LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			LEDS_DUPLEX,
			LEDS_2G5 | LEDS_LINK | LEDS_ACT },
		      {
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT | LEDS_5G,
			LEDS_LINK | LEDS_ACT | LEDS_10G,
			LEDS_2G5 | LEDS_LINK,
			LEDS_COL | LEDS_DUPLEX
		      }
		    },
};

#elif defined MACHINE_STEAMEMO_IG204_V1
__code const struct machine machine = {
	.machine_name = "Steamemo IG204 V1",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
	.phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
	.is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},
	// Left SFP port (5)
	// LED pin 9
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	// Right SFP port (6)
	// LED pin 24
	.sfp_port[1].pin_detect = GPIO50_I2C_SCL2_UART1_TX,
	.sfp_port[1].pin_los = GPIO51_I2C_SDA2_UART1_RX,
	.sfp_port[1].pin_tx_disable = GPIO_NA,
	.sfp_port[1].sds = 0,
	.sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO_NA,
	.high_leds = { .mux =  LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	.led_sets = {
			{
				// RJ45 Amber LED (left)
				LEDS_2G5 | LEDS_LINK,
				// RJ45 Green LED (Right)
				LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
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
	.led_mux = {
		0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x0f, 0x20, 0x0d, 0x0e, 0x10, 0x11, 0x12, 0x14, 0x15, 0x16, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e, 0x0c, 0x21, 0x22, 0x23
	},
};

#elif defined MACHINE_HI_K0801WS
__code const struct machine machine = {
    .machine_name = "Hi-Source HI-k0801WS",
    .isRTL8373 = 1,
    .min_port = 0,
    .max_port = 8,
    .n_sfp = 1,
    .log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
    .phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
    .is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},

    .sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
    .sfp_port[0].pin_los = GPIO37,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 1,
    .sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
    .reset_pin = GPIO_NA,

    .high_leds = {
        .mux = LED_27 | LED_29,
        .enable = LED_28_SYS | LED_29
    },

    /* Ports 1-8 use set 0, port 9 SFP uses set 1 */
    .port_led_set = {0, 0, 0, 0, 0, 0, 0, 0, 1},

    .led_sets = {
        {   /* Set 0: RJ45 copper ports
             * Amber = 2.5G
             * Green = 1G/100M/10M with activity
             */
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

#elif defined MACHINE_PCB_K0801W_V1
/*
 * PCB-K0801W-V1.0 / DIP-K0801WS-V1.0 (PCB dated 2307) - the earlier PCB
 * revision of the 8 x 2.5GBASE-T + 1 x SFP+ board sold as Hisource
 * Hi-K0801WS, whose V2.0 revision is the target above. The owner's unit
 * carries the retail label SFP-J08E-HG1, which is what .model_name reports.
 *
 * *** LED WIRING AND SFP PINS NOT VERIFIED YET ***
 *
 * The board is confirmed to boot, run and list all 9 ports correctly with
 * this target (2026-10-12, `FLASH_NO_DIO=1` for its Boya/BoHong flash), which
 * settles .isRTL8373 and .n_sfp. Everything else is still inherited unchanged
 * from the V2.0 target, because the board-specific LED/GPIO wiring cannot be
 * recovered from a stock image: a scan of the sibling 4+2 board's dump finds
 * zero writes to any 0x65xx switch register address (see
 * doc/devices/PCB-K0402W-V1.0.md). .led_mux, .led_sets, .port_led_set,
 * .high_leds and the SFP pins therefore have to be measured on the board -
 * doc/devices/PCB-K0801W-V1.0.md lists the commands that answer each one.
 *
 * Only .machine_name and .model_name differ from the V2.0 target, so a boot
 * log states unambiguously which revision is running. Expect the copper ports
 * and their order to be right; expect wrong LEDs (dark, inverted or swapped)
 * and possibly an SFP slot that reports no module. There is no reset button
 * on this board, so the built-in configuration is restored with the `defaults`
 * command or the button in the web UI.
 */
__code const struct machine machine = {
    .machine_name = "PCB-K0801W-V1.0",
    .model_name = "SFP-J08E-HG1",
    .isRTL8373 = 1,                     /* verified: boots with no 'INCORRECT MACHINE!' */
    .min_port = 0,
    .max_port = 8,
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

#elif defined(MACHINE_PCB_K0402WS_V2) || defined(MACHINE_FNS1200P)
__code const struct machine machine = {
#if defined(MACHINE_PCB_K0402WS_V2)
	.machine_name = "PCB-K0402WS-V2.0",
#else
    .machine_name = "FNS-1200P",
#endif
    .isRTL8373 = 0,
    .min_port = 3,
    .max_port = 8,
    .n_sfp = 2,
    .log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
    .phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
    .is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},

    /* Left SFP (logical 8, SDS1): GPIO30=ModAbs, GPIO37=RX_LOS */
    .sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
    .sfp_port[0].pin_los = GPIO37,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 1,
    .sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

    /* Right SFP (logical 3, SDS0): GPIO50=ModAbs, GPIO51=RX_LOS */
    .sfp_port[1].pin_detect = GPIO50_I2C_SCL2_UART1_TX,
    .sfp_port[1].pin_los = GPIO51_I2C_SDA2_UART1_RX,
    .sfp_port[1].pin_tx_disable = GPIO_NA,
    .sfp_port[1].sds = 0,
    .sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),

    .reset_pin = GPIO_NA,

    .high_leds = { .mux = LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },

    /* Copper ports use SET0; SFP ports use SET1 */
    .port_led_set = {0, 0, 0, 1, 0, 0, 0, 0, 1},

    .led_sets = {
        {   /* SET0: copper — LED0=amber (2.5G), LED2=green (1G/100M/10M) */
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

    .led_mux_custom = 1,
    .led_mux = {
        0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,  /* GPIO0-7: unused */
        0x0f, 0x0c, 0x0d, 0x0e, 0x10, 0x11, 0x12,          /* GPIO8-14 */
        0x14, 0x15, 0x16, 0x18, 0x19, 0x1a,                 /* GPIO15-20 */
        0x1c, 0x1d, 0x1e, 0x20, 0x21, 0x22, 0x23            /* GPIO21-27 */
    },
};


#elif defined MACHINE_PCB_K0402W_V1
/*
 * PCB-K0402W-V1.0 / DIP-K0402WS-V1.0 (PCB dated 2023-07), sold as Hisource
 * Hi-K0402WS: 4 x 2.5GBASE-T + 2 x SFP+, one 2 MiB SPI NOR (U2, BeyoMicro
 * 25Q16BS) and the SoC under the heatsink. The board is the earlier revision
 * of the PCB-K0402WS-V2.0/V3.0 family, so the machine parameters are taken
 * from the V2.0 block above. .machine_name is the PCB revision and
 * .model_name the retail model, which the web UI shows as 硬件 / 型号.
 *
 * Verified on the owner's boards (2026-10-11 .. 2026-10-13) against the
 * checklist in doc/devices/PCB-K0402W-V1.0.md: boots with FLASH_NO_DIO=1
 * (the Boya/BoHong part returns garbage for the Dual-I/O MMIO read mode),
 * is an RTL8372 (`regget 0004` = 0x83720000), drives 4 + 2 ports in the order
 * below (one cable per jack), lights the copper LEDs as the sets say
 * (amber = 2.5G, green = 1G/100M/10M with activity), reports and links both
 * SFP cages (module detect, RX_LOS and module EEPROM over I2C) and restores
 * the built-in configuration from the web UI.
 *
 * Only .high_leds cannot be measured on this board: the front-panel power LED
 * is hard-wired and stays lit whatever the system-LED bits say, so the
 * inherited value is kept and nothing on the panel is left undriven.
 *
 * The wiring was measured, not read out of a stock image: a scan of the stock
 * firmware finds zero writes of any 0x65xx switch register address (an
 * RTLPlayground build has 70 such sites), so .led_mux, .led_sets,
 * .port_led_set and the SFP pins are not recoverable from a dump.
 */
__code const struct machine machine = {
    .machine_name = "PCB-K0402W-V1.0",
    .model_name = "Hi-K0402WS",
    .isRTL8373 = 0,                     /* verified: 'Detecting CPU: RTL8372' */
    .min_port = 3,
    .max_port = 8,
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


#elif defined MACHINE_PCB_SWTG024AS_A_2_0_1
__code const struct machine machine = {
    .machine_name = "PCB-SWTG024AS-A-2.0.1",
    .isRTL8373 = 0,
    .min_port = 3,
    .max_port = 8,
    .n_sfp = 2,
    .log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
    .phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
    .is_sfp = {0, 0, 0, 1, 0, 0, 0, 0, 2},

    // SFP port on SDS0 / logical port 3
    .sfp_port[0].pin_detect = GPIO37,
    .sfp_port[0].pin_los = GPIO_NA,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 0,
    .sfp_port[0].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),

    // SFP port on SDS1 / logical port 8
    .sfp_port[1].pin_detect = GPIO38,
    .sfp_port[1].pin_los = GPIO_NA,
    .sfp_port[1].pin_tx_disable = GPIO_NA,
    .sfp_port[1].sds = 1,
    .sfp_port[1].i2c =  I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

    .reset_pin = GPIO_NA,
    .high_leds = { .mux =  LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },
    .port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
    .led_sets = {
                    {
                            LEDS_2G5 | LEDS_LINK | LEDS_ACT,
                            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
                            LEDS_DUPLEX,
                            LEDS_2G5 | LEDS_LINK | LEDS_ACT
                    },
                    {
                            LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT,
                            LEDS_10G | LEDS_LINK | LEDS_ACT,
                            LEDS_2G5 | LEDS_LINK,
                            LEDS_COL | LEDS_DUPLEX
                    },
     },
    .led_mux_custom = 1,
    .led_mux = {
                            0x00,0x01,0x04,0x05,0x08,0x09,0x0c,0x3f,0x0d,0x10,0x11,0x0e,0x14,0x11,0x12,0x15,0x15,0x16,0x18,0x19,0x1a,0x19,0x1d,0x1e,0x1c,0x1d,0x20,0x21
            },
    };

#elif defined MACHINE_ONT_S207CW_62TS_SE

__code const struct machine machine = {
    .machine_name = "ONT-S207CW-62TS-SE / Binardat 2G06-04210GSM / Binardat 2G06-04210GS",
    .isRTL8373 = 0,
    .min_port = 3,
    .max_port = 8,
    .n_sfp = 2,
    .log_to_phys_port = {0, 0, 0, 5, 4, 3, 2, 1, 6},
    .phys_to_log_port = {7, 6, 5, 4, 3, 8, 0, 0, 0},
    .is_sfp = {0, 0, 0, 1, 0, 0, 0, 0, 2},

    // SFP port on SDS0 / logical port 3
    .sfp_port[0].pin_detect = GPIO37,
    .sfp_port[0].pin_los = GPIO_NA,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 0,
    .sfp_port[0].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),

    // SFP port on SDS1 / logical port 8
    .sfp_port[1].pin_detect = GPIO38,
    .sfp_port[1].pin_los = GPIO_NA,
    .sfp_port[1].pin_tx_disable = GPIO_NA,
    .sfp_port[1].sds = 1,
    .sfp_port[1].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

    .reset_pin = GPIO_NA,
    .high_leds = { .mux =  LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },
    .port_led_set = { 0, 0, 0, 3, 0, 0, 2, 2, 3},
    .mac_flash_offset = 0x1FC000,
    .led_sets = {
        { // RJ45 Ports 3-4 (normal wiring)
            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
            LEDS_2G5 | LEDS_LINK | LEDS_ACT,
            0,
            0
        },
        { // Unused
            0,
            0,
            0,
            0
        },
        { // RJ45 Ports 1-2 (reversed wiring)
            LEDS_2G5 | LEDS_LINK | LEDS_ACT,
            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
            0,
            0
        },
        { // SFP Ports 5-6
            LEDS_10G | LEDS_LINK | LEDS_ACT,
            LEDS_2G5 | LEDS_1G | LEDS_LINK | LEDS_ACT,
            0,
            0
        },
    },
    .led_mux_custom = 1,
    .led_mux = {
        0x00,0x01,0x04,0x05,0x08,0x09,0x0c,0x3f,0x0d,0x10,0x11,0x0e,0x14,0x11,0x12,0x15,0x15,0x16,0x18,0x19,0x1a,0x19,0x1d,0x1e,0x1c,0x1d,0x20,0x21
    },
};

#elif defined MACHINE_SWTG024AS_A_2_0_1_5C_1SFP
__code const struct machine machine = {
    .machine_name = "SWTG024AS-A-V2.0.1-5C-1SFP",
    .isRTL8373 = 0,
    .min_port = 3,
    .max_port = 8,
    .n_sfp = 1,
    .log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
    .phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
    .is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO38,
    .sfp_port[0].pin_los = GPIO_NA,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 1,
    .sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

    .reset_pin = GPIO_NA,
    .high_leds = { .mux =  LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },
    .port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	/* Ports 1-5 RJ45 use set 0, port 9 SFP uses set 1 
	 * Ports 1-5: Green: 2.5GBit, Amber: 10/100/1000MBit
	 * SFP-port: Blue: 10GBit, Green: 100MBit-2.5GBit
	 */
    .led_sets = {
                    {
                            LEDS_2G5 | LEDS_LINK | LEDS_ACT,
                            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
                            LEDS_DUPLEX,
                            LEDS_2G5 | LEDS_LINK | LEDS_ACT
                    },
                    {
                            LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT,
                            LEDS_10G | LEDS_LINK | LEDS_ACT,
                            LEDS_2G5 | LEDS_LINK,
                            LEDS_COL | LEDS_DUPLEX
                    },
     },
    .led_mux_custom = 1,
    .led_mux = {
                            0x00,0x01,0x04,0x05,0x08,0x09,0x0c,0x3f,0x0d,0x10,0x11,0x0e,0x14,0x11,0x12,0x15,0x15,0x16,0x18,0x19,0x1a,0x19,0x1d,0x1e,0x1c,0x1d,0x20,0x21
            },
    };

#elif defined(MACHINE_SWTG024AS_V2_0) || defined(MACHINE_SKS3200M_4GPY2XF)
/*
 * SWTG024AS-Vx.y - 4 x 2.5GBASE-T + 2 x SFP (RTL8372). V2.0 is sold as
 * Lianguo/Horaco/Sodola SL-SWTG124AS, V1.0 as XikeStor SKS3200M-4GPY2XF
 * (2 MiB flash, label CM-23-08-2043 023-16721). Both revisions take the same
 * wiring, so they share this block and differ in how they name the board: the
 * XikeStor target puts the PCB revision in .machine_name and the retail model
 * in .model_name, which the web UI shows as 硬件 (hardware) and 型号 (model).
 */
__code const struct machine machine = {
#if defined(MACHINE_SKS3200M_4GPY2XF)
    .machine_name = "PCB-SWTG024AS-V1.0",
    .model_name = "SKS3200M-4GPY2XF",
#else
    .machine_name = "SWTG024AS-V2.0",
#endif
    .isRTL8373 = 0,
#if defined(MACHINE_SKS3200M_4GPY2XF)
    /*
     * The stock firmware of this unit reported MAC 1C:2A:A3:23:E4:5E, so the
     * board keeps its factory MAC in flash the way the ZX-SWTGW215AS boards do.
     * Only this target reads it - for the V2.0 boards of the family that area
     * of the flash is unverified.
     */
    .mac_flash_offset = 0x1FC000,
#endif
    .min_port = 3,
    .max_port = 8,
    .n_sfp = 1,
    .log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
    .phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
    .is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
    .sfp_port[0].pin_los = GPIO37,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 1,
    .sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

#if defined(MACHINE_SKS3200M_4GPY2XF)
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
#else
    .reset_pin = GPIO_NA,
#endif
    .high_leds = { .mux =  LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },
    .port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	/* Ports 1-5 RJ45 use set 0, port 9 SFP uses set 1 
	 * Ports 1-5: Green: 2.5GBit, Amber: 10/100/1000MBit
	 * SFP-port: Blue: 10GBit, Amber: 100MBit-2.5GBit
	 */
    .led_sets = {
                    {
                            LEDS_2G5 | LEDS_LINK | LEDS_ACT,
                            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
                            LEDS_DUPLEX,
                            LEDS_2G5 | LEDS_LINK | LEDS_ACT
                    },
                    {
                            LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT,
                            LEDS_10G | LEDS_LINK | LEDS_ACT,
                            LEDS_2G5 | LEDS_LINK,
                            LEDS_COL | LEDS_DUPLEX
                    },
     },
    .led_mux_custom = 1,
    .led_mux = {
                            0x00,0x01,0x04,0x05,0x08,0x09,0x0c,0x3f,0x0d,0x10,0x11,0x0e,0x14,0x11,0x12,0x15,0x15,0x16,0x18,0x19,0x1a,0x19,0x1d,0x1e,0x1c,0x1d,0x20,0x21
            },
    };

#elif defined MACHINE_ZX310S_4T2XT
__code const struct machine machine = {
	.machine_name = "ZX310S_4T2XT",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 0,
	.n_10g = 2,
	.log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
	.phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 0},
	.reset_pin = GPIO48_I2C_SCL1,
	.high_leds = { .mux = LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },
	.led_mux_custom = 1,
	.led_mux = { 0x00, 0x01, 0x04, 0x05, 0x08, // 65e0
		     0x09, 0x0c, 0x3f, 0x0d, 0x10, // 65e4
		     0x11, 0x0e, 0x14, 0x11, 0x12, // 65e8
		     0x15, 0x15, 0x16, 0x18, 0x19, // 65ec
		     0x1a, 0x19, 0x1d, 0x1e, 0x1c, // 65f0
		     0x1d, 0x20, 0x21 },
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	/* Ports 1-4: Green: 2.5GBit, Orange: 10/100/1000MBit
	 * Ports 5-6: Green: 10GBit, Orange: <= 5GBit
	 */
	.led_sets = { { LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			LEDS_DUPLEX,
			LEDS_2G5 | LEDS_LINK | LEDS_ACT },
		      {
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT,
			LEDS_LINK | LEDS_ACT | LEDS_10G | LEDS_5G,
			LEDS_2G5 | LEDS_LINK,
			LEDS_COL | LEDS_DUPLEX
		      }
		    },
};

#elif defined MACHINE_FG_4GT_2SX_V2_0
__code const struct machine machine = {
	.machine_name = "FG-4GT-2SX_V2.0",
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 2,
	.log_to_phys_port = {0, 0, 0, 6, 1, 2, 3, 4, 5},
	.phys_to_log_port = {4, 5, 6, 7, 8, 3, 0, 0, 0},
	.is_sfp = {0, 0, 0, 2, 0, 0, 0, 0, 1},
	
	// Left SFP port
	.sfp_port[0].pin_detect = GPIO38, 
	.sfp_port[0].pin_los = GPIO_NA, 
	.sfp_port[0].sds = 1, 
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

	// Right SFP port
	.sfp_port[1].pin_detect = GPIO37,
	.sfp_port[1].pin_los = GPIO_NA, 
	.sfp_port[1].sds = 0, 
	.sfp_port[1].i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ),

	.reset_pin = GPIO_NA,
	.high_leds = { .mux = LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	/* Ports 1-4 RJ45 use set 0, port 5-6 SFP uses set 1 
	 * Ports 1-4: Green: 2.5GBit, Amber: 10/100/1000MBit
	 * Ports 5-6: Green: 100MBit-10GBit
	 */
	.led_sets = { 
			{
				LEDS_2G5 | LEDS_LINK | LEDS_ACT,
				LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
				0,
				LEDS_2G5 | LEDS_LINK | LEDS_ACT
			},
			{  
				LEDS_10G | LEDS_5G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
				LEDS_10G | LEDS_LINK,
				0,
				LEDS_COL | LEDS_DUPLEX
			},
			{
				LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
				LEDS_2G5 | LEDS_1G | LEDS_LINK,
				LEDS_5G | LEDS_2G5 | LEDS_LINK | LEDS_ACT,
				LEDS_10G | LEDS_LINK | LEDS_ACT
			},
			{
				LEDS_TX,
				LEDS_RX,
				LEDS_10G | LEDS_TWO_PAIR_5G | LEDS_5G | LEDS_TWO_PAIR_2G5 |
					LEDS_2G5 | LEDS_TWO_PAIR_1G | LEDS_1G | LEDS_500M | LEDS_100M | LEDS_10M | LEDS_ACT,
				LEDS_10G | LEDS_TWO_PAIR_5G | LEDS_5G | LEDS_TWO_PAIR_2G5 |
					LEDS_2G5 | LEDS_TWO_PAIR_1G | LEDS_1G | LEDS_500M | LEDS_100M | LEDS_10M | LEDS_LINK
			},
	 },
	.led_mux_custom = 1,
	.led_mux = {
				0x0c, 0x0d, 0x0e, 0x10, 0x11, 0x12, 0x14, 0x3f, 0x15, 0x16,
				0x18, 0x0e, 0x19, 0x11, 0x12, 0x1a, 0x15, 0x16, 0x1c, 0x19,
				0x1a, 0x1d, 0x1d, 0x1e, 0x1e, 0x20, 0x21, 0x22
		},
	};

#elif defined MACHINE_FG_8GT_1SX
__code const struct machine machine = {
	.machine_name = "FG-8GT-1SX",
	.isRTL8373 = 1,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO38,
	.sfp_port[0].pin_los = GPIO_NA,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO_NA,
	.high_leds = { .mux = LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.led_sets = {
		/*
			Ports 1-8: Left Amber 10/100/1000MBit (Vendor no 10M, but we add it)
					   Right Green 2.5GBit
			SFP-port: Any speed
		*/
		{
			LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			0,
			0,
		},
		{
			LEDS_10G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			0,
			0,
			0,
		},
	},
	.led_mux_custom = 1,
	.led_mux = {
		0x00, 0x01, 0x04, 0x05, 0x08, 0x09, 0x0c, 0x09, 0x0d, 0x10,
		0x11, 0x0e, 0x14, 0x11, 0x12, 0x15, 0x15, 0x16, 0x18, 0x19,
		0x1a, 0x19, 0x1d, 0x1e, 0x1c, 0x1d, 0x20, 0x21,
	},
};

#elif defined MACHINE_LIANGUO_HYWS_SGT0108S
__code const struct machine machine = {
    .machine_name = "Lianguo HYWS-SGT0108S",
    .isRTL8373 = 1,
    .min_port = 0,
    .max_port = 8,
    .n_sfp = 1,
    .log_to_phys_port = {2, 1, 4, 3, 5, 6, 7, 8, 9},
    .phys_to_log_port = {1, 0, 3, 2, 4, 5, 6, 7, 8},
    .is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},

    .sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
    .sfp_port[0].pin_los = GPIO37,
    .sfp_port[0].pin_tx_disable = GPIO_NA,
    .sfp_port[0].sds = 1,
    .sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

    .reset_pin = GPIO_NA,
	.high_leds = { .mux = 0, .enable = 0 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	.led_sets = { 
		{ 
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT, // RJ45 Blue LEDs
			LEDS_2G5 | LEDS_LINK, // RJ45 White LEDs
			0,
			0
		},
		{ 
			LEDS_10G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT, // SFP blue LED?
			0,
			0,
			0
		},
	},
};
#elif defined MACHINE_POE_2G080110GS
__code const struct machine machine = {
	.machine_name = "MokerLink POE-2G080110GS",
	.isRTL8373 = 1,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO_NA,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	.led_sets = {
		{
			/* SET0: RJ45  2.5G = amber, 1G/100M/10M = green */
			LEDS_2G5 | LEDS_LINK | LEDS_ACT, /* amber */
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT, /* green */
			0,
			0
		},
		{
			/* SET1: SFP+ port = green */
			LEDS_2G5 | LEDS_TWO_PAIR_1G | LEDS_1G | LEDS_500M | LEDS_100M | LEDS_10M |
				LEDS_LINK | LEDS_ACT | LEDS_10G | LEDS_TWO_PAIR_5G | LEDS_5G | LEDS_TWO_PAIR_2G5,
			0,
			0,
			0
		}
	},
	.led_mux_custom = 1,
	.led_mux = { 0x00, 0x01, 0x04, 0x05, 0x08,
			0x09, 0x0c, 0x09, 0x0d, 0x10,
			0x11, 0x0e, 0x14, 0x11, 0x12,
			0x15, 0x15, 0x16, 0x18, 0x19,
			0x1a, 0x19, 0x1d, 0x1e, 0x1c,
			0x1d, 0x20, 0x21 },
};
#elif defined MACHINE_PB_2132
__code const struct machine machine = {
	.machine_name = "PB-2132",
	.isRTL8373 = 1,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},

	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

	.reset_pin = GPIO_NA,

	.high_leds = { .mux = LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},

	.led_sets = {
		{
			/* LED0 = RJ45: Amber LED: 2.5G */
			LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			/* LED1 = RJ45: Green LED: 10M / 100M / 1000M */
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			0,
			0
		},
		{
			/* SFP+ single Green LED. */
			LEDS_10G | LEDS_5G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT,
			0,
			0,
			0
		},
	},
 };

#elif defined MACHINE_HASIVO_S600W_4GT_2XGT_SE
__code const struct machine machine = {
    .machine_name = "Hasivo S600W-4GT-2XGT-SE",
    .isRTL8373 = 0,
    .min_port = 3,
    .max_port = 8,
    .n_sfp = 0,
    .n_10g = 2,
    .log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
    .phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
    .is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 0},
    .reset_pin = GPIO54_ACL_BIT2_EN,
    .high_leds = { .mux = LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },
    .port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1 },
    /* Ports 1-4: Green: 2.5GBit, Orange: 10/100/1000MBit
     * Ports 5-6: Green: 10GBit, Orange: 5/2.5GBit, None: 10/100/1000MBit
     */
    .led_sets = {
        {
            LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT, // Yellow
            LEDS_2G5 | LEDS_LINK, // Green
            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK, // Orange
            0, // Unused
        },
        {
            LEDS_10G | LEDS_5G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT, // Yellow
            LEDS_10G | LEDS_LINK, // Green
            LEDS_5G | LEDS_2G5 | LEDS_LINK, // Orange
            0, // Unused
        },
    },
    .led_mux_custom = 1,
    .led_mux = {
        0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
        0x3f, 0x3f, 0x3f, 0x0f, 0x20,
        0x21, 0x22, 0x10, 0x11, 0x12,
        0x14, 0x15, 0x16, 0x18, 0x19,
        0x1a, 0x1c, 0x1d, 0x1e, 0x0c,
        0x0d, 0x0e, 0x23
    },

};
#elif defined MACHINE_HASIVO_S1100WP_8GT_1SX_SE
__code const struct machine machine = {
	.machine_name = "Hasivo S1100WP-8GT-1SX-SE",
	.isRTL8373 = 1,
	.mac_flash_offset = 0x1FC000,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO54_ACL_BIT2_EN,
	.high_leds = { .mux = LED_27 | LED_28_SYS | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	.led_sets = { { LEDS_2G5 | LEDS_LINK | LEDS_ACT, // Green LED (right)
					0,
					LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT, // Amber LED (left)
					0
				  },
				  { LEDS_10G | LEDS_5G | LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT, // SFP LED
					0, 
					0,
					0
				  },
				},
};
#elif defined MACHINE_F7008_2_5
__code const struct machine machine = {
	.machine_name = "F7008-2.5 8+1 SFP Port Switch",
	.isRTL8373 = 1,
	.min_port = 0,
	.max_port = 8,
	.n_sfp = 1,
	.log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	.phys_to_log_port = {0, 1, 2, 3, 4, 5, 6, 7, 8},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.sfp_port[0].pin_detect = GPIO30_ACL_BIT3_EN,
	.sfp_port[0].pin_los = GPIO37,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO_NA,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = {0, 0, 0, 0, 0, 0, 0, 0, 1},
	.led_sets = {
		{	/* SET0 */
			LEDS_2G5 | LEDS_LINK,				/* LEDID0 - green: 2.5G link */
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,	/* LEDID1 - yellow: traffic */
			0,
			0,
		},
		{	/* SET1 - SFP+ */
			LEDS_10G | LEDS_5G | LEDS_2G5 | LEDS_LINK,
			LEDS_10G | LEDS_5G | LEDS_2G5 | LEDS_1G | LEDS_LINK | LEDS_ACT,
			0,
			0,
		},
	},
	.led_mux_custom = 1,
	.led_mux = {
		0x00, 0x01, 0x04, 0x05, 0x08, 0x09, 0x0c, 0x02,
		0x0d, 0x10, 0x11, 0x02, 0x14, 0x02, 0x02, 0x15,
		0x02, 0x02, 0x18, 0x02, 0x02, 0x19, 0x02, 0x02,
		0x1c, 0x1d, 0x02, 0x02,
	},
};

#else
	#error "Please select a machine type in machine.h"
#endif
