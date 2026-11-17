#include "machine_defs.h"

void machine_custom_init(void) __banked
{
	/* The RTL8372 reset value of LED_GLB_IO_EN is 0x77ffff00, which leaves
	 * LED pads 0-7 disabled - they stay GPIO inputs. The PB-2131 panel does
	 * not wire a lamp to pads 0-7, but the mux drives them to their inactive
	 * level, so enable every pad: on a variant that does use them that keeps
	 * the lamp off instead of leaving the pad floating. */
	REG_SET(RTL837X_REG_LED_GLB_IO_EN, 0x7fffffffUL);

	/* Front-panel "VLAN" slide switch on GPIO36. gpio_input_setup() also
	 * sets PIN_MUX_1 bit 30: with that bit clear the pin level does not
	 * reach the input register and the switch reads 0 in both positions
	 * (measured). vlan_switch_poll() reads it from the main loop. */
	gpio_input_setup(GPIO36_PWM_OUT);
}
