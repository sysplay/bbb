#include "serial.h"
#include "debug.h"

void debug_init(void)
{
	serial_init(115200);
}
void debug_shut(void)
{
	serial_shut();
}

void print_nl(void)
{
	serial_tx("\r\n");
}
void print_str(char *str)
{
	serial_tx(str);
}
void print_str_nl(char *str)
{
	serial_tx(str);
	serial_tx("\r\n");
}
void print_num(uint32_t n)
{
	int sig, i, dig;
	char num[11];

	sig = 9;
	for (i = 9; i >= 0; i--)
	{
		dig = n % 10;
		if (dig)
			sig = i;
		num[i] = '0' + dig;
		n /= 10;
	}
	num[10] = 0;
	serial_tx(num + sig);
}
void print_hex(uint32_t n)
{
	int i;
	char c;

	serial_byte_tx('0');
	serial_byte_tx('x');
	for (i = 7; i >= 0; i--)
	{
		c = ((n >> (i << 2)) & 0xF);
		c += ((c <= 9) ? '0' : (-10 + 'A'));
		serial_byte_tx(c);
	}
}
char scan_char(void)
{
	return serial_byte_rx();
}
void scan_line(char *line, int max_len)
{
	serial_rx(line, max_len);
}
