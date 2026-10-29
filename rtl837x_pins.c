#include "rtl837x_pins.h"
#include "rtl837x_common.h"
#include "rtl837x_sfr.h"
#include "rtl837x_regs.h"
#include "rtl837x_port.h"
#include "machine.h"

extern __code const struct machine machine;
extern __xdata uint8_t sfr_data[4];
extern __xdata uint8_t outbuf[TCP_OUTBUF_SIZE];
extern __xdata uint16_t slen;

#pragma codeseg BANK2
#pragma constseg BANK2

/* Returns RTL837X_REG_GPIO_XX_OUTPUT register address */
static uint16_t gpio_output_reg(uint8_t pin) __banked{
	return pin < 32 ? RTL837X_REG_GPIO_00_31_OUTPUT : RTL837X_REG_GPIO_32_63_OUTPUT;
} 

/* Returns RTL837X_REG_GPIO_XX_DIRECTION register address */
static uint16_t gpio_direction_reg(uint8_t pin) __banked {
	return pin < 32 ? RTL837X_REG_GPIO_00_31_DIRECTION : RTL837X_REG_GPIO_32_63_DIRECTION;
} 


/* Enable GPIO functions for pin */
static void gpio_mux_setup(uint8_t pin) __banked
{
	// Some GPIOs require setting MUX registers to enable GPIO
	switch (pin) {
		case GPIO10_LED10:
			reg_bit_clear(RTL837X_PIN_MUX_0, 10);
			break;
		case GPIO30_ACL_BIT3_EN:
			reg_bit_clear(RTL837X_PIN_MUX_2, 3);
			break;
		case GPIO36_PWM_OUT:
			reg_bit_set(RTL837X_PIN_MUX_1, 30);
			break;
		case GPIO37:
		case GPIO38:
			// Intentionally empty, always GPIO
			break;
		case GPIO46_I2C_SCL0:
			// Bit 7-8 0b00 -> GPIO
			reg_read_m(RTL837X_PIN_MUX_1);
			sfr_mask_data(0, 0x80, 0x00);
			sfr_mask_data(1, 0x01, 0x00);
			reg_write_m(RTL837X_PIN_MUX_1);	
			break;
		case GPIO50_I2C_SCL2_UART1_TX:
			// Bit 15-16 0b00 -> GPIO
			reg_read_m(RTL837X_PIN_MUX_1);
			sfr_mask_data(1, 0x80, 0x00);
			sfr_mask_data(2, 0x01, 0x00);
			reg_write_m(RTL837X_PIN_MUX_1);	
			break;
		case GPIO51_I2C_SDA2_UART1_RX:
			// Bit 17-18 0b00 -> GPIO
			reg_read_m(RTL837X_PIN_MUX_1);
			sfr_mask_data(2, 0x06, 0x00);
			reg_write_m(RTL837X_PIN_MUX_1);
			break;
		case GPIO54_ACL_BIT2_EN:
			reg_bit_clear(RTL837X_PIN_MUX_2, 2);
			break;
		case GPIO_NA:
			print_string("Attemped to assign GPIO function to N/A pin!");
			break;
		default:
			print_string("GPIO MUX setup not implemented for pin="); print_byte(pin); print_string("\n");
	}
}

void gpio_input_setup(uint8_t pin) __banked {
	if (pin == GPIO_NA) { 
		return;
	}

	gpio_mux_setup(pin);
	reg_bit_clear(gpio_direction_reg(pin), (pin % 32));
}

void gpio_output_setup(uint8_t pin, __xdata uint8_t initial_val) __banked{
	if (pin == GPIO_NA) { 
		return;
	}
	gpio_mux_setup(pin);

	// We need to setup value before enabling output on PIN
	if (initial_val) {
		reg_bit_set(gpio_output_reg(pin), (pin % 32));
	} else {
		reg_bit_clear(gpio_output_reg(pin), (pin % 32));
	}

	reg_bit_set(gpio_direction_reg(pin), (pin % 32));
}


/*
 * Front-panel "VLAN" slide switch, on boards that have one (.vlan_switch_pin).
 * While it is on, the non-SFP ports are isolated from each other: each keeps a
 * path to the SFP cages and to the CPU port, so the uplink and the management
 * interface keep working. Called from the main loop; the isolation registers
 * are only rewritten when the switch changes.
 *
 * Lives here rather than in rtl837x_port.c because BANK1 is full on this
 * machine; port_isolate() is reached through its bank trampoline.
 */
__xdata uint8_t vlan_switch_last = 0xff;	/* 0/1 = applied, 0xfe = function off */
extern __xdata uint8_t vlan_switch_enabled;

void vlan_switch_poll(void) __banked
{
	uint8_t state, port;
	__xdata uint16_t all = 0;
	__xdata uint16_t mask;

	if (!machine.vlan_switch_pin)
		return;

	if (vlan_switch_enabled) {
		state = gpio_pin_test(machine.vlan_switch_pin) ? 1 : 0;
		if (state == vlan_switch_last)
			return;
	} else {
		/* Switching the function off hands the isolation back to the
		 * web configuration: restore "allow all" once, then leave the
		 * isolation registers alone. */
		if (vlan_switch_last == 0xfe)
			return;
		state = 0xfe;
	}

	/* Every port of this machine plus the CPU port */
	for (port = machine.min_port; port <= machine.max_port; port++)
		all |= ((uint16_t)1) << port;
	all |= ((uint16_t)1) << 9;	/* CPU port */

	/* While the switch is on, an RJ45 port may reach everything except the
	 * other RJ45 ports: the SFP cages and the CPU port stay reachable. */
	mask = all;
	if (state == 1) {
		for (port = machine.min_port; port <= machine.max_port; port++)
			if (!machine.is_sfp[port])
				mask &= ~(((uint16_t)1) << port);
	}

	for (port = machine.min_port; port <= machine.max_port; port++) {
		if (machine.is_sfp[port])
			continue;
		port_isolate(port, mask);
	}
	vlan_switch_last = state;

	print_string("\nVLAN switch: ");
	if (state == 1)
		print_string("ON, the RJ45 ports are isolated from each other\n");
	else if (state == 0)
		print_string("OFF, all ports may talk to each other\n");
	else
		print_string("function off, isolation left to the web configuration\n");
}


/* Append the "vlan_switch" / "vlan_switch_en" fields to /information.json.
 * Lives here, like vlan_switch_poll(), because page_impl.c is BANK1 and full. */
void vlan_switch_json(void) __banked
{
	slen += strtox(outbuf + slen, "\",\"vlan_switch\":\"");
	slen += strtox(outbuf + slen, gpio_pin_test(machine.vlan_switch_pin) ? "on" : "off");
	slen += strtox(outbuf + slen, "\",\"vlan_switch_en\":\"");
	slen += strtox(outbuf + slen, vlan_switch_enabled ? "on" : "off");
}


/*
 * Read up to 16 consecutive registers of the EEPROM via I2C into sfp_buf
 */
bool sfp_read_block(uint8_t slot, uint8_t reg, uint8_t len) __banked __reentrant
{
	uint8_t dev;
	uint8_t val;

	len--;
	if (len > 15)
		return false;

	dev = (reg & 0x80) ? 0x51 : 0x50;	// 0x51 holds the diagnostics, 0x50 the module data
	reg &= 0x7f;

	REG_WRITE(RTL837X_REG_I2C_IN, 0, 0, 0, reg);

	REG_WRITE(RTL837X_REG_I2C_CTRL, 0x00,
		  0x1 << (I2C_MEM_ADDR_WIDTH - 16) | len,
		  (dev >> 5) | machine.sfp_port[slot].i2c,
		  ((dev << 3) & 0xff) | 0x1);

	do {
		reg_read(RTL837X_REG_I2C_CTRL);
	} while (SFR_DATA_0 & 0x1);

	if (SFR_DATA_0 & 0x2)
		return false;

	for (uint8_t i = 0; i <= len; i++) {
		switch (i & 0x3) {
		case 0:
			reg_read(RTL837X_REG_I2C_OUT + i);
			val = SFR_DATA_0;
			break;
		case 1:
			val = SFR_DATA_8;
			break;
		case 2:
			val = SFR_DATA_16;
			break;
		default:
			val = SFR_DATA_24;
			break;
		}
		sfp_buf[i] = val;
	}

	return true;
}
