#include "machine_init_defs.h"

void machine_custom_init(void) __banked
{
	/* Same hook as the upstream PCB_K0402W_V3_0 target: LED pad 6 is
	 * disabled at reset and carries a panel lamp on this board. */
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 6);
}
