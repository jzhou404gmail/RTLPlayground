#include "machine_defs.h"

void machine_custom_init(void) __banked
{
	/* The RTL8372 reset value of LED_GLB_IO_EN is 0x77ffff00, which leaves
	 * LED pads 0-7 disabled. This board's panel lamps are on pads 0-27
	 * (SFP2 on 0-1, then the four RJ45 jacks, SFP1 on 24-27), so enable
	 * every pad. Without this the whole panel stays dark. */
	REG_SET(RTL837X_REG_LED_GLB_IO_EN, 0x7fffffffUL);
}
