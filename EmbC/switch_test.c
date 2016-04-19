#include "debug.h"
#include "bbb.h"

int c_entry(void)
{
	debug_init();

	scan_char();
	print_str_nl("Welcome to SysPlay");

	CM_PER_GPIO2_CLKCTRL |= (0x1 << 18) | (0x2 << 0);
	GPIO2_FALLINGDETECT |= (1 << 8);
	//GPIO2_RISINGDETECT |= (1 << 8);
	GPIO2_IRQSTATUS_SET_0 = (1 << 8);

	for (;;)
	{
		print_str("Press any key to get switch status: ");
		scan_char();
		print_str("Switch St: ");
		print_hex(GPIO2_IRQSTATUS_0);
		print_nl();
		GPIO2_IRQSTATUS_0 = (1 << 8);
	}

	GPIO2_IRQSTATUS_CLR_0 = (1 << 8);
	GPIO2_RISINGDETECT &= ~(1 << 8);
	GPIO2_FALLINGDETECT &= ~(1 << 8);
	CM_PER_GPIO2_CLKCTRL &= ~((0x1 << 18) | (0x2 << 0));

	debug_shut();

	return 0;
}
