#ifndef MY_SERIAL_H
#define MY_SERIAL_H

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>

struct uart_omap_port {
	struct uart_port	port;
	struct device		*dev;

	unsigned char		ier;
	unsigned char		lcr;
	unsigned char		mcr;
	unsigned char		fcr;
	unsigned char		efr;
	unsigned char		dll;
	unsigned char		dlh;
	unsigned char		mdr1;
	unsigned char		scr;
	unsigned char		wer;

	/*
	 * Some bits in registers are cleared on a read, so they must
	 * be saved whenever the register is read but the bits will not
	 * be immediately processed.
	 */
	char			name[20];
	u8			wakeups_enabled;
	u32			features;

	u32			latency;
	u32			calc_latency;
	char *rx_buff;
	size_t rx_size;
	char *tx_buff;
	size_t tx_size;

	dev_t devt;
	struct cdev c_dev;
	struct class *cl;
};

int serial_read(struct uart_omap_port *up, size_t len);
int fcd_init(struct uart_omap_port *up);
void fcd_exit(void);
int serial_open(struct uart_port *port);
void serial_omap_shutdown(struct uart_port *port);
void serial_omap_start_tx(struct uart_port *port);
int serial_omap_startup(struct uart_port *port);

#endif
