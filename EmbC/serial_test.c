#include "serial.h"

int c_entry(void)
{
	char str[32];

	serial_init(115200);

	serial_byte_rx();
	serial_tx("Welcome to SysPlay\r\n");

	for (;;)
	{
		serial_tx("Cmd> ");
		serial_rx(str, 32);
		serial_tx("You entered: ");
		serial_tx(str);
		serial_tx("\r\n");
	}

	serial_shut();

	return 0;
}
