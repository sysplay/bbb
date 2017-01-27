#include "debug.h"
#include "leds.h"
#include "switch.h"

#ifdef INTR_BASED_SWITCH
void handler(void)
{
	if (switch_pressed()) // This also clears the switch intr status
	{
		print_str("Switch: ");
		if (switch_read() == sw_released)
			print_str_nl("Released");
		else
			print_str_nl("Pressed");

		leds_toggle(0);
	}
}
#endif

int c_entry(void)
{
	debug_init();
	leds_init();

	scan_char();
	print_str_nl("Welcome to SysPlay");

#ifdef INTR_BASED_SWITCH
	interrupt_init();
	switch_handler_register(handler);
#endif
	switch_init();

	for (;;)
	{
#ifdef INTR_BASED_SWITCH
		delay(1000);
		print_str("> ");
		scan_char();
#else
		if (switch_pressed())
		{
			print_str("Switch: ");
			if (switch_read() == sw_released)
				print_str_nl("Released");
			else
				print_str_nl("Pressed");

			leds_toggle(0);
		}
#endif
	}

	switch_shut();
#ifdef INTR_BASED_SWITCH
	switch_handler_unregister();
	interrupt_shut();
#endif
	leds_shut();
	debug_shut();

	return 0;
}
