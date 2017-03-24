#include "debug.h"
#include "leds.h"
#include "timer.h"
#ifdef INTR_BASED_TIMER
#include "interrupt.h"
#endif
#include "bbb.h"

#ifdef INTR_BASED_TIMER
void handler(void)
{
	TIMER0_IRQSTATUS = (1 << 1);
	leds_toggle(3);
}
#endif

int c_entry(void)
{
	debug_init();
	leds_init();

	scan_char();
	print_str_nl("Welcome to SysPlay");

#ifdef INTR_BASED_TIMER
	interrupt_init();
	timer_handler_register(handler);
#endif
	timer_init(1000);

	/* TODO
#ifdef INTR_BASED_TIMER
	print_str("Interrupt Vector @ ");
	print_hex((uint32_t)(interrupt_vectors_address_get()));
	print_nl();
#endif
	 */

	for (;;)
	{
		/*
		print_str("Press any key: ");
		scan_char();
		print_str("TCLR: ");
		print_hex(TIMER0_TCLR);
		print_str("; TLDR: ");
		print_hex(TIMER0_TLDR);
		print_str("; TCRR: ");
		print_hex(TIMER0_TCRR);
		print_str("; IRQ St Raw: ");
		print_hex(TIMER0_IRQSTATUS_RAW);
		print_str("; IRQ St: ");
		print_hex(TIMER0_IRQSTATUS);
		print_str("; Gl IRQ Mask: ");
		print_hex(INTC_MIR2);
		print_str("; Gl IRQ St: ");
		print_hex(INTC_PENDING_IRQ2);
		print_str("; IRQ Num: ");
		print_hex(INTC_SIR_IRQ);
		print_nl();
		 */
#ifdef INTR_BASED_TIMER
		delay(1000);
#else
		if (TIMER0_IRQSTATUS & (1 << 1))
		{
			TIMER0_IRQSTATUS = (1 << 1);
			leds_toggle(3);
		}
#endif
	}

	timer_shut();
#ifdef INTR_BASED_TIMER
	timer_handler_unregister();
	interrupt_shut();
#endif
	leds_shut();
	debug_shut();

	return 0;
}
