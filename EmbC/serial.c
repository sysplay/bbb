#include "bbb.h"
#include "common.h"
#include "serial.h"

void serial_init(uint32_t baud)
{
	uint16_t dl;

	// Somehow already done - possibly by the stage 0 bootloader
	//CM_WKUP_UART0_CLKCTRL = (0x2 << 0); // Enable the wakeup module

	//UART0_MDR1 = (0x7 << 0); // Disable UART
	//UART0_LCR = 0x00; // Switch to "Reg Op Mode" to configure Intr Enables
	//UART0_IER = (1 << 1) | (1 << 0); // THR | RHR
	UART0_LCR = 0xB7; // Switch to "Reg Cfg Mode B" to configure Divisor Latch, Parity, SB, Charlen
	// DLL = (48000000 / 16 / baud) & 0xFF;
	// DLH = ((48000000 / 16 / baud) >> 8) & 0xFF;
	switch (baud)
	{
		case 300:
			dl = 10000;
			break;
		case 600:
			dl = 5000;
			break;
		case 1200:
			dl = 2500;
			break;
		case 2400:
			dl = 1250;
			break;
		case 4800:
			dl = 625;
			break;
		case 9600:
			dl = 312;
			break;
		case 14400:
			dl = 208;
			break;
		case 19200:
			dl = 156;
			break;
		case 28800:
			dl = 104;
			break;
		case 38400:
			dl = 78;
			break;
		case 57600:
			dl = 52;
			break;
		case 115200:
			dl = 26;
			break;
		case 230400:
			dl = 13;
			break;
		case 460800:
			dl = 8;
			break;
		case 921600:
			dl = 4;
			break;
		case 1843000:
			dl = 2;
			break;
		case 3688400:
			dl = 1;
			break;
	}
	UART0_DLL = (dl) & 0xFF;
	UART0_DLH = (dl >> 8) & 0xFF;
	UART0_LCR = (0 << 3) | (0 << 2) | (0x3 << 0); // N18 for 8N1
	UART0_MDR1 = (0x0 << 0); // Enable UART 16x mode
}
void serial_shut(void)
{
	UART0_MDR1 = (0x7 << 0); // Disable UART
	UART0_LCR = 0xB7; // Switch to "Reg Cfg Mode B" to reset Divisor Latch, Parity, SB, Charlen
	UART0_DLL = 0;
	UART0_DLH = 0;
	UART0_LCR = 0;
	//UART0_LCR = 0x00; // Switch to "Reg Op Mode" to configure Intr Enables
	//UART0_IER = 0;
}

void serial_byte_tx(uint8_t byte)
{
	UART0_THR = byte;

	while (!(UART0_LSR & (1 << 5)))
		;
}
int serial_byte_available(void)
{
	return (!(!(UART0_LSR & (1 << 0))));
}
uint8_t serial_byte_rx(void)
{
	while (!(UART0_LSR & (1 << 0)))
		;

	return UART0_RHR;
}

void serial_tx(char* str)
{
	while (*str)
	{
		serial_byte_tx(*str++);
	}
}
void serial_rx(char *str, int max_len)
{
	int i;

	for (i = 0; i < max_len - 1; i++)
	{
		str[i] = serial_byte_rx();
		if (str[i] == '\r')
			break;
	}
	str[i] = 0;
}
