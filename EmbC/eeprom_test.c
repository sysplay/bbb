#include "common.h"
#include "debug.h"
#include "eeprom.h"

int c_entry(void)
{
	union
	{
		uint8_t c[4];
		uint32_t i;
	} u;
	uint8_t *addr = (uint8_t *)(0);

	debug_init();
	scan_char();
	print_str_nl("Welcome to SysPlay");

	eeprom_init();

	for (;;)
	{
		print_str("Press any key: ");
		scan_char();
		print_nl();
		print_str("EEPROM Content: ");
		if (eeprom_read(addr, u.c, sizeof(u)) != 0)
		{
			print_str_nl("Read Error");
		}
		else
		{
			print_hex(u.i);
			print_nl();
		}
	}

	eeprom_shut();
	debug_shut();

	return 0;
}
