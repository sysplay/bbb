#include "serial.h"
#include "debug.h"
#include "common.h"

#define MAX_FILE_SIZE (16 * 1024)
#define BLOCKS_CNT (MAX_FILE_SIZE / 128)

#define ACK '\006'
#define NAK '\025'

#define SOH '\001'
#define EOT '\004'

#define SUB '\032'

static char hdr[3], data[BLOCKS_CNT][128], csum;

void print_hex8(uint8_t c)
{
	uint8_t nib;

	nib = (c >> 4) & 0xF;
	if (nib < 10)
		serial_byte_tx('0' + nib);
	else
		serial_byte_tx('A' + (nib - 10));
	nib = c & 0xF;
	if (nib < 10)
		serial_byte_tx('0' + nib);
	else
		serial_byte_tx('A' + (nib - 10));
}

int c_entry(void)
{
	int data_avail, i, j;
	uint8_t bno, last_cnt, sum, retries;
	char e[5], d[5];
	void (*f)(void) = (void (*)(void))((void *)(data));

	serial_init(115200);

	serial_byte_rx();
	serial_tx("Welcome to SysPlay\r\n");

	for (;;)
	{
		serial_tx("Press just one <Enter> and send the data/leds_test.bin file (<= ");
		print_num(MAX_FILE_SIZE);
		serial_tx(" bytes) thru xmodem within 10 secs\r\n");
		serial_byte_rx();
		delay(15000000); // ~10 sec
		serial_byte_tx(NAK);

		bno = 0;
		last_cnt = 128;
		retries = 0;
		while (1)
		{
			hdr[0] = serial_byte_rx();
			if (hdr[0] == EOT)
			{
				serial_byte_tx(ACK);
				break;
			}
			else if (hdr[0] != SOH)
			{
				e[retries] = '0';
				d[retries] = hdr[0];
				retries++;
				serial_byte_tx(NAK);
				continue;
			}
			hdr[1] = serial_byte_rx();
			if (hdr[1] != (bno + 1))
			{
				e[retries] = '1';
				d[retries] = hdr[1];
				retries++;
				serial_byte_tx(NAK);
				continue;
			}
			hdr[2] = serial_byte_rx();
			if (hdr[2] != (255 - (bno + 1)))
			{
				e[retries] = '2';
				d[retries] = hdr[2];
				retries++;
				serial_byte_tx(NAK);
				continue;
			}
			sum = 0;
			for (i = 0; i < 128; i++)
			{
				sum += data[bno][i] = serial_byte_rx();
				if (data[bno][i] == SUB)
				{
					if (last_cnt == 128) // found the last_cnt
						last_cnt = i;
				}
				else
				{
					if (last_cnt != 128) // reset the last_cnt
						last_cnt = 128;
				}
			}
			csum = serial_byte_rx();
			if (csum != sum)
			{
				e[retries] = 'c';
				d[retries] = csum ^ sum;
				retries++;
				serial_byte_tx(NAK);
				continue;
			}
			bno++;
			serial_byte_tx(ACK);
		}
		serial_byte_rx();
		serial_tx("File received w/ ");
		print_num(bno);
		serial_tx(" block(s) with ");
		print_num(128 * (bno - 1) + last_cnt);
		serial_tx(" byte(s) with ");
		print_num(retries);
		serial_tx(" retries(s)\r\n");
		for (j = 0; j < bno; j++)
			for (i = 0; i < 128; i++)
			{
				print_hex8(data[j][i]);
				if ((i + 1) & 0x1F) // % 32
					serial_byte_tx(' ');
				else
					serial_tx("\r\n");
			}
		if (retries)
		{
			serial_tx("Retry Reasons:\r\n");
			for (i = 0; i < retries; i++)
			{
				serial_byte_tx(e[i]);
				serial_tx(": ");
				print_hex8(d[i]);
				serial_byte_tx(';');
			}
			serial_tx("\r\n");
		}
#if 0
		serial_tx("Now executing the downloaded program ...\r\n");
		(*f)();
#endif
	}

	serial_shut();

	return 0;
}
