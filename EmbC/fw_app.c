/*
 * Command Requirements of the firmware:
 * + l 0 - Switch off LED 0
 * + l 1 - Switch on LED 0
 * + b <msecs> - Blink LED 1 at an interval of <msecs> milliseconds
 * + b 0 - Disable the blink of LED 1
 * + r <addr> <cnt> - Read <acnt> bytes from EEPROM starting at address <addr>
 */
#include "debug.h"
#include "leds.h"
#include "timer.h"
#include "eeprom.h"

static void init_shell(void)
{
	debug_init();

	leds_init();
	eeprom_init();

	scan_char();
}
static void shut_shell(void)
{
	eeprom_shut();
	leds_shut();

	debug_shut();
}

int atoi(char *str, int *next)
{
	int i = 0, num = 0;

	while ((str[i]) && ((str[i] == ' ') || (str[i] == '\t')))
		i++;

	if (!str[i])
	{
		if (next)
			*next = i;
		return num;
	}

	while ((str[i] >= '0') && (str[i] <= '9'))
	{
		// TODO: Fill in your code for atoi implementation

		i++;
	}

	if (next)
		*next = i;

	return num;
}

static void loop_forever(void)
{
	char cmd[128];
	int st, cnt, next;
	uint32_t msecs; 
	uint8_t *addr;

	print_str("$ ");
	scan_line(cmd, 128);

	switch (cmd[0])
	{
		case 0: // Ignore empty string
			break;
		case 'l':
			st = atoi(cmd + 1, NULL);
			// TODO: Fill in your code for LED implementation
			break;
		case 'b':
			msecs = atoi(cmd + 1, NULL);
			// TODO: Fill in your code for timer implementation
			break;
		case 'r':
			addr = (uint8_t *)(atoi(cmd + 1, &next));
			cnt = atoi(cmd + 1 + next, NULL);
			// TODO: Fill in your code for EEPROM implementation
			break;
		default:
			print_str("Invalid cmd: ");
			print_str(cmd);
			print_nl();
	}
}

int c_entry(void)
{
	init_shell();

	for (;;)
		loop_forever();

	shut_shell();

	return 0;
}
