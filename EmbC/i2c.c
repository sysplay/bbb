#include "bbb.h"
#include "i2c.h"
//#include "interrupt.h"

#define I2C_ST_BB (1 << 12) // Bus Busy

#define I2C_ST_TR (1 << 4) // Ready for accepting data for Tx
#define I2C_ST_RR (1 << 3) // Ready for reading data from Rx
#define I2C_ST_AR (1 << 2) // Ready for accessing registers

#define I2C_CON_STOP (1 << 1) // Stop condition
#define I2C_CON_START (1 << 0) // Start condition

#define I2C_QUIT_OP { send_stop(); return -1; }

static void handler(void) // TODO
{
}

void i2c_init(I2CMode mode)
{
	CM_WKUP_I2C0_CLKCTRL = (0x2 << 0); // Enable the wakeup module - has to be the first one before accessing any I2C
	// I2C Pin Settings for SDA & SCL as per the reference code
	CTRL_MOD_CONF_I2C0_SDA &= ~0x0F; // 0x30
	CTRL_MOD_CONF_I2C0_SCL &= ~0x0F; // 0x30

#define SCLK 48 // System Clock in MHz
	I2C0_PSC =  SCLK / 24 - 1; // Set up approx. 24MHz module clock (ICLK) = SCLK / (PSC + 1)
	/*
	 * Set up I2C clock for 100Kbps or 400Kbps = 1/t
	 * 1/800K = tLow = (SCLL + 7) * 1/ICLK => SCLL = 24M / 800K - 7
	 * 1/800K = tHigh = (SCLH + 5) * 1/ICLK => SCLH = 24M / 800K - 5
	 */
	I2C0_SCLL = 23;
	I2C0_SCLH = 25;

	I2C0_CON = (0 << 12) | // Standard / Fast mode
				(0 << 7); // 7-bit own address

	I2C0_CON |= (1 << 15); // Enable I2C

	//interrupt_init();
	//interrupt_handler_register(I2C0_IRQ, handler);
	I2C0_IRQENABLE_SET |= I2C_ST_TR | I2C_ST_RR | I2C_ST_AR; // Enable various status
}
void i2c_shut(void)
{
	I2C0_IRQENABLE_CLR |= I2C_ST_TR | I2C_ST_RR | I2C_ST_AR; // Disable various status
	//interrupt_handler_unregister(I2C0_IRQ);
	//interrupt_shut();

	I2C0_CON = 0; // Disable I2C

	CM_WKUP_I2C0_CLKCTRL = (0x0 << 0); // Disable the wakeup module
}

static int send_start(void)
{
	while (I2C0_IRQSTATUS & I2C_ST_BB) // Wait for bus not busy
		;
	I2C0_CON |= I2C_CON_START; // Trigger start
	while (I2C0_CON & I2C_CON_START) // Wait for start complete
		;
	return 0;
}
static int send_restart(void)
{
	I2C0_CON |= I2C_CON_START; // Trigger start again
	while (I2C0_CON & I2C_CON_START) // Wait for start complete
		;
	return 0;
}
static void send_stop(void)
{
	I2C0_CON |= I2C_CON_STOP; // Trigger stop
	while (I2C0_CON & I2C_CON_STOP) // Wait for stop complete
		;
}
static int send_data(uint8_t data)
{
	while (!(I2C0_IRQSTATUS & I2C_ST_TR)) // Wait for ready for accepting data for Tx
		;
	I2C0_DATA = data; // Send data
	I2C0_IRQSTATUS |= I2C_ST_TR; // Clear ready for accepting data for Tx
	return 0;
}
static int recv_data(uint8_t *data)
{
	while (!(I2C0_IRQSTATUS & I2C_ST_RR)) // Wait for ready for reading data from Rx
		;
	*data = I2C0_DATA; // Read data
	I2C0_IRQSTATUS |= I2C_ST_RR; // Clear ready for reading data from Rx
	return 0;
}
int i2c_master_tx(uint8_t addr, uint8_t *data, int len)
{
	int i;

	I2C0_SA = addr; // Set the slave addr

	I2C0_CNT = len & 0xFF; // Set the data count
	I2C0_CON |= (1 << 9) | (1 << 10); // Set the Master Tx mode
	if (send_start()) I2C_QUIT_OP; // Trigger by sending Start
	for (i = 0; i < len; i++)
	{
		if (send_data(data[i])) I2C_QUIT_OP;
	}
	send_stop(); // Done, so Stop
	return 0;
}
int i2c_master_rx(uint8_t addr, uint8_t *data, int len)
{
	int i;

	I2C0_SA = addr; // Set the slave addr

	I2C0_CNT = len & 0xFF; // Set the data count
	I2C0_CON |= (1 << 10); // Set the Master mode
	I2C0_CON &= ~(1 << 9); // Set the Rx mode
	if (send_start()) I2C_QUIT_OP; // Trigger by sending Start
	for (i = 0; i < len; i++)
	{
		if (recv_data(&data[i])) I2C_QUIT_OP;
	}
	send_stop(); // Done, so Stop
	return 0;
}
int i2c_master_tx_rx(uint8_t addr, uint8_t *tx_data, int tx_len, uint8_t *rx_data, int rx_len)
{
	int i;

	I2C0_SA = addr; // Set the slave addr

	I2C0_CNT = tx_len & 0xFF; // Set the tx data count
	I2C0_CON |= (1 << 9) | (1 << 10); // Set the Master Tx mode
	if (send_start()) I2C_QUIT_OP; // Trigger by sending Start
	for (i = 0; i < tx_len; i++) // Send Data
	{
		if (send_data(tx_data[i])) I2C_QUIT_OP;
	}
	while (!(I2C0_IRQSTATUS & I2C_ST_AR)) // Wait for ready for accessing registers after the tx complete
		;
	I2C0_IRQSTATUS |= I2C_ST_AR;
	I2C0_CNT = rx_len & 0xFF; // Set the rx data count
	I2C0_CON &= ~(1 << 9); // Set the Master Rx mode - note master is already set
	if (send_restart()) I2C_QUIT_OP; // Trigger by sending Start again
	for (i = 0; i < rx_len; i++) // Receive Data
	{
		if (recv_data(&rx_data[i])) I2C_QUIT_OP;
	}
	send_stop(); // Done, so Stop
	return 0;
}
