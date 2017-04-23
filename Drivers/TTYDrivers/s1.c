/*
 * Driver for OMAP-UART controller.
 * Based on drivers/serial/8250.c
 *
 * Copyright (C) 2010 Texas Instruments.
 *
 * Authors:
 *	Govindraj R	<govindraj.raja@ti.com>
 *	Thara Gopinath	<thara@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Note: This driver is made separate from 8250 driver as we cannot
 * over load 8250 driver with omap platform specific configuration for
 * features like DMA, it makes easier to implement features like DMA and
 * hardware flow control and software flow control configuration with
 * this driver as required for the omap-platform.
 */

#if defined(CONFIG_SERIAL_OMAP_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/serial_reg.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/serial_core.h>
#include <linux/irq.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/wait.h>
#include <linux/platform_data/serial-omap.h>
#include "char_serial.h"

#include <dt-bindings/gpio/gpio.h>


#define OMAP_UART_TX_WAKEUP_EN		BIT(7)

/* Feature flags */
#define OMAP_UART_WER_HAS_TX_WAKEUP	BIT(0)


#define DEFAULT_CLK_SPEED 48000000 /* 48Mhz*/

/* SCR register bitmasks */
#define OMAP_UART_SCR_RX_TRIG_GRANU1_MASK		(1 << 7)
#define OMAP_UART_SCR_TX_TRIG_GRANU1_MASK		(1 << 6)
#define OMAP_UART_SCR_TX_EMPTY			(1 << 3)

/* FCR register bitmasks */
#define OMAP_UART_FCR_RX_FIFO_TRIG_MASK			(0x3 << 6)
#define OMAP_UART_FCR_TX_FIFO_TRIG_MASK			(0x3 << 4)


/* WER = 0x7F
 * Enable module level wakeup in WER reg
 */
#define OMAP_UART_WER_MOD_WKUP	0X7F

#define OMAP_UART_TCR_TRIG	0x0F
#define DRIVER_NAME1	"my_uart"
#define FUNC_ENTER() do { printk(KERN_INFO "Enter: %s\n", __func__); } while (0)

#define to_uart_omap_port(p)	((container_of((p), struct uart_omap_port, port)))

static void
serial_omap_set_termios(struct uart_port *port, struct ktermios *termios,
			struct ktermios *old);

static char start_flg = 0;

static inline unsigned int serial_in(struct uart_omap_port *up, int offset)
{
	offset <<= up->port.regshift;
	return readw(up->port.membase + offset);
}

static inline void serial_out(struct uart_omap_port *up, int offset, int value)
{
	offset <<= up->port.regshift;
	writew(value, up->port.membase + offset);
}
/* 
 * APIs
 * serial_out - Writing the uart registers
 * serial_in  - Reading from the uart registers
 *
 */

/* 
 * Registers & MASK
 * UART_LSR - Line Status Register
 * UART_TX -  Xmit Register
 * UART_LSR_THRE - Transmitter FIFO Empty mask
 * UART_LSR_DR   - RX FIFO Empty mask
 *
 */
static void transmit_chars(struct uart_omap_port *up, unsigned int lsr)
{
	/* 
	 * 1. Wait for UART_LSR_THRE bit to be set
	 * 2. Write into the UART_TX register
	 *
	 */
	int cnt = 10;
	while (!(serial_in(up, UART_LSR) & UART_LSR_THRE) && cnt--)
			msleep(10);
	if (cnt <= 0) {
			printk("TX timed out\n");
			return;
	}
	serial_out(up, UART_TX, 'p');
}

void serial_omap_start_tx(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	FUNC_ENTER();
	transmit_chars(up, 0);
}

int serial_read(struct uart_omap_port *up, size_t len)
{
	/* 
	* 1. Wait for UART_LSR_DR to be set
	* 2. Read the data from UART_RX register
	*
	*/
	int cnt = 10;
	while (!(serial_in(up, UART_LSR) & UART_LSR_DR) && cnt--)
			msleep(10);
	if (cnt <= 0) {
			printk("RX timed out\n");
			return -ETIMEDOUT;
	}
	printk("Char = %c\n", serial_in(up, UART_RX));
	return 1;
}		

int serial_omap_startup(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	struct ktermios termios;

	if (start_flg)
		return 0;
	start_flg = 1;

	FUNC_ENTER();

	spin_lock_init(&up->port.lock);

/*
	 * Clear the FIFO buffers and disable them.
	 * (they will be reenabled in set_termios())
	 */
	serial_out(up, UART_FCR, 0);

	/*
	 * Clear the interrupt registers.
	 */
	(void) serial_in(up, UART_LSR);
	if (serial_in(up, UART_LSR) & UART_LSR_DR)
		(void) serial_in(up, UART_RX);
	(void) serial_in(up, UART_IIR);
	(void) serial_in(up, UART_MSR);

	/*
	 * Now, initialize the UART
	 */
	serial_out(up, UART_LCR, UART_LCR_WLEN8);
	/* Disable Interrupts */
	up->ier = 0;
	/* Enable module level wake up */
	up->wer = OMAP_UART_WER_MOD_WKUP;
	if (up->features & OMAP_UART_WER_HAS_TX_WAKEUP)
		up->wer |= OMAP_UART_TX_WAKEUP_EN;

	serial_out(up, UART_OMAP_WER, up->wer);

	memset(&termios, 0, sizeof(struct ktermios));
	termios.c_cflag = CS8 | B115200 | CREAD;
	termios.c_iflag = IGNBRK | IGNPAR;
	serial_omap_set_termios(&up->port, &termios, NULL);
	up->rx_size = 10;
	up->rx_buff = devm_kzalloc(up->dev, up->rx_size, GFP_KERNEL);
	up->tx_size = 10;
	up->tx_buff = devm_kzalloc(up->dev, up->rx_size, GFP_KERNEL);
	return 0;
}

void serial_omap_shutdown(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	start_flg = 0;
	FUNC_ENTER();

	dev_dbg(up->port.dev, "serial_omap_shutdown+%d\n", up->port.line);

	/*
	 * Disable interrupts from this port
	 */
	up->ier = 0;
	serial_out(up, UART_IER, 0);


	/*
	 * Read data port to reset things, and then free the irq
	 */
	if (serial_in(up, UART_LSR) & UART_LSR_DR)
		(void) serial_in(up, UART_RX);

}


static void
serial_omap_set_termios(struct uart_port *port, struct ktermios *termios,
			struct ktermios *old)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	unsigned char cval = 0;
	unsigned long flags = 0;
	unsigned int baud, quot;
	FUNC_ENTER();
	printk("Termios Cflag = %x\n", termios->c_cflag);
	printk("Termios iflag = %x\n", termios->c_iflag);

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		cval = UART_LCR_WLEN5;
		break;
	case CS6:
		cval = UART_LCR_WLEN6;
		break;
	case CS7:
		cval = UART_LCR_WLEN7;
		break;
	default:
	case CS8:
		cval = UART_LCR_WLEN8;
		break;
	}

	if (termios->c_cflag & CSTOPB)
		cval |= UART_LCR_STOP;
	if (termios->c_cflag & PARENB)
		cval |= UART_LCR_PARITY;
	if (!(termios->c_cflag & PARODD))
		cval |= UART_LCR_EPAR;
	if (termios->c_cflag & CMSPAR)
		cval |= UART_LCR_SPAR;

	/*
	 * Ask the core to calculate the divisor for us.
	 */

	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/13);
	quot = 13;

	up->dll = quot & 0xff;
	up->dlh = quot >> 8;
	up->mdr1 = UART_OMAP_MDR1_DISABLE;

	up->fcr = UART_FCR_R_TRIG_01 | UART_FCR_T_TRIG_01 |
			UART_FCR_ENABLE_FIFO;

	/*
	 * Ok, we're now changing the port state. Do it with
	 * interrupts disabled.
	 */
	spin_lock_irqsave(&up->port.lock, flags);
	serial_out(up, UART_IER, up->ier);
	serial_out(up, UART_LCR, cval);		/* reset DLAB */
	up->lcr = cval;
	up->scr = 0;

	/* FIFOs and DMA Settings */

	/* FCR can be changed only when the
	 * baud clock is not running
	 * DLL_REG and DLH_REG set to 0.
	 */
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_out(up, UART_DLL, 0);
	serial_out(up, UART_DLM, 0);
	serial_out(up, UART_LCR, 0);
	up->fcr = 0;
	up->efr = 0;

	serial_out(up, UART_FCR, up->fcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	serial_out(up, UART_OMAP_SCR, up->scr);

	/* Reset UART_MCR_TCRTLR: this must be done with the EFR_ECB bit set */
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_out(up, UART_MCR, up->mcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);

	/* Protocol, Baud Rate, and Interrupt Settings */
	serial_out(up, UART_OMAP_MDR1, up->mdr1);

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr | UART_EFR_ECB);

	serial_out(up, UART_LCR, 0);
	serial_out(up, UART_IER, 0);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	serial_out(up, UART_DLL, up->dll);	/* LS of divisor */
	serial_out(up, UART_DLM, up->dlh);	/* MS of divisor */

	serial_out(up, UART_LCR, 0);
	serial_out(up, UART_IER, up->ier);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, cval);

	up->mdr1 = UART_OMAP_MDR1_13X_MODE;
	serial_out(up, UART_OMAP_MDR1, up->mdr1);

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);

	serial_out(up, UART_MCR, up->mcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, up->lcr);

	spin_unlock_irqrestore(&up->port.lock, flags);
	dev_dbg(up->port.dev, "serial_omap_set_termios+%d\n", up->port.line);
}

static struct omap_uart_port_info *of_get_uart_port_info(struct device *dev)
{
	struct omap_uart_port_info *omap_up_info;

	omap_up_info = devm_kzalloc(dev, sizeof(*omap_up_info), GFP_KERNEL);
	if (!omap_up_info)
		return NULL; /* out of memory */

	of_property_read_u32(dev->of_node, "clock-frequency",
					 &omap_up_info->uartclk);
	return omap_up_info;
}


static int serial_omap_probe(struct platform_device *pdev)
{
	struct uart_omap_port	*up;
	struct resource		*mem;
	struct omap_uart_port_info *omap_up_info = dev_get_platdata(&pdev->dev);
	int ret;

	if (pdev->dev.of_node) {
		omap_up_info = of_get_uart_port_info(&pdev->dev);
		pdev->dev.platform_data = omap_up_info;
	}
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}


	if (!devm_request_mem_region(&pdev->dev, mem->start, resource_size(mem),
				pdev->dev.driver->name)) {
		dev_err(&pdev->dev, "memory region already claimed\n");
		return -EBUSY;
	}
	up = devm_kzalloc(&pdev->dev, sizeof(*up), GFP_KERNEL);
	if (!up)
		return -ENOMEM;

	up->dev = &pdev->dev;
	up->port.dev = &pdev->dev;
	up->port.type = PORT_16550A;
	up->port.iotype = UPIO_MEM;

	up->port.regshift = 2;

	sprintf(up->name, "MY UART%d", up->port.line);
	up->port.mapbase = mem->start;
	up->port.membase = devm_ioremap(&pdev->dev, mem->start,
						resource_size(mem));
	if (!up->port.membase) {
		dev_err(&pdev->dev, "can't ioremap UART\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	up->port.flags = omap_up_info->flags;
	up->port.uartclk = omap_up_info->uartclk;
	if (!up->port.uartclk) {
		up->port.uartclk = DEFAULT_CLK_SPEED;
		dev_warn(&pdev->dev, "No clock speed specified: using default:"
						"%d\n", DEFAULT_CLK_SPEED);
	}

	platform_set_drvdata(pdev, up);

	fcd_init(up);
	return 0;

err_ioremap:
	dev_err(&pdev->dev, "[UART%d]: failure [%s]: %d\n",
				pdev->id, __func__, ret);
	return ret;
}

static int serial_omap_remove(struct platform_device *dev)
{
	fcd_exit();

	return 0;
}


#if defined(CONFIG_OF)
static const struct of_device_id omap_serial_of_match[] = {
	{ .compatible = "ti,my-uart" },
	{},
};
MODULE_DEVICE_TABLE(of, omap_serial_of_match);
#endif

static struct platform_driver serial_omap_driver = {
	.probe          = serial_omap_probe,
	.remove         = serial_omap_remove,
	.driver		= {
		.name	= DRIVER_NAME1,
		.pm	= NULL,
		.of_match_table = of_match_ptr(omap_serial_of_match),
	},
};

static int __init serial_omap_init(void)
{
	int ret;
	ret = platform_driver_register(&serial_omap_driver);
	return ret;
}

static void __exit serial_omap_exit(void)
{
	platform_driver_unregister(&serial_omap_driver);
}

module_init(serial_omap_init);
module_exit(serial_omap_exit);

MODULE_DESCRIPTION("OMAP High Speed UART driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Texas Instruments Inc");
