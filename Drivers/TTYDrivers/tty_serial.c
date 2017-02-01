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
#include <linux/platform_data/serial-omap.h>

#include <dt-bindings/gpio/gpio.h>

#define OMAP_MAX_HSUART_PORTS	6

#define UART_BUILD_REVISION(x, y)	(((x) << 8) | (y))

#define OMAP_UART_REV_42 0x0402
#define OMAP_UART_REV_46 0x0406
#define OMAP_UART_REV_52 0x0502
#define OMAP_UART_REV_63 0x0603

#define OMAP_UART_TX_WAKEUP_EN		BIT(7)

/* Feature flags */
#define OMAP_UART_WER_HAS_TX_WAKEUP	BIT(0)

#define UART_ERRATA_i202_MDR1_ACCESS	BIT(0)
#define UART_ERRATA_i291_DMA_FORCEIDLE	BIT(1)

#define DEFAULT_CLK_SPEED 48000000 /* 48Mhz*/

/* SCR register bitmasks */
#define OMAP_UART_SCR_RX_TRIG_GRANU1_MASK		(1 << 7)
#define OMAP_UART_SCR_TX_TRIG_GRANU1_MASK		(1 << 6)
#define OMAP_UART_SCR_TX_EMPTY			(1 << 3)

/* FCR register bitmasks */
#define OMAP_UART_FCR_RX_FIFO_TRIG_MASK			(0x3 << 6)
#define OMAP_UART_FCR_TX_FIFO_TRIG_MASK			(0x3 << 4)

/* MVR register bitmasks */
#define OMAP_UART_MVR_SCHEME_SHIFT	30

#define OMAP_UART_LEGACY_MVR_MAJ_MASK	0xf0
#define OMAP_UART_LEGACY_MVR_MAJ_SHIFT	4
#define OMAP_UART_LEGACY_MVR_MIN_MASK	0x0f

#define OMAP_UART_MVR_MAJ_MASK		0x700
#define OMAP_UART_MVR_MAJ_SHIFT		8
#define OMAP_UART_MVR_MIN_MASK		0x3f

#define OMAP_UART_DMA_CH_FREE	-1

#define MSR_SAVE_FLAGS		UART_MSR_ANY_DELTA
#define OMAP_MODE13X_SPEED	230400

/* WER = 0x7F
 * Enable module level wakeup in WER reg
 */
#define OMAP_UART_WER_MOD_WKUP	0X7F

/* Enable XON/XOFF flow control on output */
#define OMAP_UART_SW_TX		0x08

/* Enable XON/XOFF flow control on input */
#define OMAP_UART_SW_RX		0x02

#define OMAP_UART_SW_CLR	0xF0

#define OMAP_UART_TCR_TRIG	0x0F
#define DRIVER_NAME1	"my_uart"
#define FUNC_ENTER() do { printk(KERN_INFO "Enter: %s\n", __func__); } while (0)

struct uart_omap_dma {
	u8			uart_dma_tx;
	u8			uart_dma_rx;
	int			rx_dma_channel;
	int			tx_dma_channel;
	dma_addr_t		rx_buf_dma_phys;
	dma_addr_t		tx_buf_dma_phys;
	unsigned int		uart_base;
	/*
	 * Buffer for rx dma.It is not required for tx because the buffer
	 * comes from port structure.
	 */
	unsigned char		*rx_buf;
	unsigned int		prev_rx_dma_pos;
	int			tx_buf_size;
	int			tx_dma_used;
	int			rx_dma_used;
	spinlock_t		tx_lock;
	spinlock_t		rx_lock;
	/* timer to poll activity on rx dma */
	struct timer_list	rx_timer;
	unsigned int		rx_buf_size;
	unsigned int		rx_poll_rate;
	unsigned int		rx_timeout;
};

struct uart_omap_port {
	struct uart_port	port;
	struct uart_omap_dma	uart_dma;
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

	int			use_dma;
	/*
	 * Some bits in registers are cleared on a read, so they must
	 * be saved whenever the register is read but the bits will not
	 * be immediately processed.
	 */
	unsigned int		lsr_break_flag;
	unsigned char		msr_saved_flags;
	char			name[20];
	unsigned long		port_activity;
	int			context_loss_cnt;
	u32			errata;
	u8			wakeups_enabled;
	u32			features;

	int			DTR_gpio;
	int			DTR_inverted;
	int			DTR_active;

	struct serial_rs485	rs485;
	int			rts_gpio;

	struct pm_qos_request	pm_qos_request;
	u32			latency;
	u32			calc_latency;
	struct work_struct	qos_work;
	bool			is_suspending;
};

#define to_uart_omap_port(p)	((container_of((p), struct uart_omap_port, port)))

static struct uart_omap_port *ui[OMAP_MAX_HSUART_PORTS];


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

static inline void serial_omap_clear_fifos(struct uart_omap_port *up)
{
	FUNC_ENTER();
	serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO);
	serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO |
		       UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
	serial_out(up, UART_FCR, 0);
}


/*
 * serial_omap_baud_is_mode16 - check if baud rate is MODE16X
 * @port: uart port info
 * @baud: baudrate for which mode needs to be determined
 *
 * Returns true if baud rate is MODE16X and false if MODE13X
 * Original table in OMAP TRM named "UART Mode Baud Rates, Divisor Values,
 * and Error Rates" determines modes not for all common baud rates.
 * E.g. for 1000000 baud rate mode must be 16x, but according to that
 * table it's determined as 13x.
 */
static bool
serial_omap_baud_is_mode16(struct uart_port *port, unsigned int baud)
{
	FUNC_ENTER();
	unsigned int n13 = port->uartclk / (13 * baud);
	unsigned int n16 = port->uartclk / (16 * baud);
	int baudAbsDiff13 = baud - (port->uartclk / (13 * n13));
	int baudAbsDiff16 = baud - (port->uartclk / (16 * n16));
	if(baudAbsDiff13 < 0)
		baudAbsDiff13 = -baudAbsDiff13;
	if(baudAbsDiff16 < 0)
		baudAbsDiff16 = -baudAbsDiff16;

	return (baudAbsDiff13 > baudAbsDiff16);
}

/*
 * serial_omap_get_divisor - calculate divisor value
 * @port: uart port info
 * @baud: baudrate for which divisor needs to be calculated.
 */
static unsigned int
serial_omap_get_divisor(struct uart_port *port, unsigned int baud)
{
	FUNC_ENTER();
	unsigned int divisor;

	if (!serial_omap_baud_is_mode16(port, baud))
		divisor = 13;
	else
		divisor = 16;
	return port->uartclk/(baud * divisor);
}


static void serial_omap_stop_tx(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	FUNC_ENTER();

	if (up->ier & UART_IER_THRI) {
		up->ier &= ~UART_IER_THRI;
		serial_out(up, UART_IER, up->ier);
	}
}

static void serial_omap_stop_rx(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	FUNC_ENTER();

	up->ier &= ~UART_IER_RLSI;
	up->port.read_status_mask &= ~UART_LSR_DR;
	serial_out(up, UART_IER, up->ier);
}

static void transmit_chars(struct uart_omap_port *up, unsigned int lsr)
{
	struct circ_buf *xmit = &up->port.state->xmit;
	int count;
	FUNC_ENTER();

	if (up->port.x_char) {
		serial_out(up, UART_TX, up->port.x_char);
		up->port.icount.tx++;
		up->port.x_char = 0;
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&up->port)) {
		serial_omap_stop_tx(&up->port);
		return;
	}
	count = up->port.fifosize / 4;
	do {
		serial_out(up, UART_TX, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		up->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS) {
		spin_unlock(&up->port.lock);
		uart_write_wakeup(&up->port);
		spin_lock(&up->port.lock);
	}

	if (uart_circ_empty(xmit))
		serial_omap_stop_tx(&up->port);
}

static inline void serial_omap_enable_ier_thri(struct uart_omap_port *up)
{
	FUNC_ENTER();
	if (!(up->ier & UART_IER_THRI)) {
		up->ier |= UART_IER_THRI;
		serial_out(up, UART_IER, up->ier);
	}
}

static void serial_omap_start_tx(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	FUNC_ENTER();
	serial_omap_enable_ier_thri(up);
}

static void serial_omap_rlsi(struct uart_omap_port *up, unsigned int lsr)
{
	unsigned int flag;
	unsigned char ch = 0;
	FUNC_ENTER();

	if (likely(lsr & UART_LSR_DR))
		ch = serial_in(up, UART_RX);

	up->port.icount.rx++;
	flag = TTY_NORMAL;

	if (lsr & UART_LSR_BI) {
		flag = TTY_BREAK;
		lsr &= ~(UART_LSR_FE | UART_LSR_PE);
		up->port.icount.brk++;
		/*
		 * We do the SysRQ and SAK checking
		 * here because otherwise the break
		 * may get masked by ignore_status_mask
		 * or read_status_mask.
		 */
		if (uart_handle_break(&up->port))
			return;

	}

	if (lsr & UART_LSR_PE) {
		flag = TTY_PARITY;
		up->port.icount.parity++;
	}

	if (lsr & UART_LSR_FE) {
		flag = TTY_FRAME;
		up->port.icount.frame++;
	}

	if (lsr & UART_LSR_OE)
		up->port.icount.overrun++;

#ifdef CONFIG_SERIAL_OMAP_CONSOLE
	if (up->port.line == up->port.cons->index) {
		/* Recover the break flag from console xmit */
		lsr |= up->lsr_break_flag;
	}
#endif
	uart_insert_char(&up->port, lsr, UART_LSR_OE, 0, flag);
}

static void serial_omap_rdi(struct uart_omap_port *up, unsigned int lsr)
{
	unsigned char ch = 0;
	unsigned int flag;
	FUNC_ENTER();

	if (!(lsr & UART_LSR_DR))
		return;

	ch = serial_in(up, UART_RX);
	flag = TTY_NORMAL;
	up->port.icount.rx++;

	if (uart_handle_sysrq_char(&up->port, ch))
		return;

	uart_insert_char(&up->port, lsr, UART_LSR_OE, ch, flag);
}

/**
 * serial_omap_irq() - This handles the interrupt from one port
 * @irq: uart port irq number
 * @dev_id: uart port info
 */
static irqreturn_t serial_omap_irq(int irq, void *dev_id)
{
	struct uart_omap_port *up = dev_id;
	unsigned int iir, lsr;
	unsigned int type;
	irqreturn_t ret = IRQ_NONE;
	int max_count = 256;
	FUNC_ENTER();

	spin_lock(&up->port.lock);

	do {
		iir = serial_in(up, UART_IIR);
		if (iir & UART_IIR_NO_INT)
			break;

		ret = IRQ_HANDLED;
		lsr = serial_in(up, UART_LSR);

		/* extract IRQ type from IIR register */
		type = iir & 0x3e;
		printk("type = %x\n", type);

		switch (type) {
#if 0 
		case UART_IIR_MSI:
			check_modem_status(up);
			break;
#endif
		case UART_IIR_THRI:
			transmit_chars(up, lsr);
			break;
		case UART_IIR_RX_TIMEOUT:
			/* FALLTHROUGH */
		case UART_IIR_RDI:
			serial_omap_rdi(up, lsr);
			break;
		case UART_IIR_RLSI:
			serial_omap_rlsi(up, lsr);
			break;
		case UART_IIR_CTS_RTS_DSR:
			/* simply try again */
			break;
		case UART_IIR_XOFF:
			/* FALLTHROUGH */
		default:
			break;
		}
	} while (!(iir & UART_IIR_NO_INT) && max_count--);

	spin_unlock(&up->port.lock);

	tty_flip_buffer_push(&up->port.state->port);

	up->port_activity = jiffies;

	return ret;
}

static unsigned int serial_omap_tx_empty(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	unsigned long flags = 0;
	unsigned int ret = 0;
	FUNC_ENTER();

	dev_dbg(up->port.dev, "serial_omap_tx_empty+%d\n", up->port.line);
	spin_lock_irqsave(&up->port.lock, flags);
	ret = serial_in(up, UART_LSR) & UART_LSR_TEMT ? TIOCSER_TEMT : 0;
	spin_unlock_irqrestore(&up->port.lock, flags);
	return ret;
}

static unsigned int serial_omap_get_mctrl(struct uart_port *port)
{
	return 0;
}

static void serial_omap_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
}

static int serial_omap_startup(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	unsigned long flags = 0;
	int retval;
	FUNC_ENTER();

	/*
	 * Allocate the IRQ
	 */
	retval = request_irq(up->port.irq, serial_omap_irq, up->port.irqflags,
				up->name, up);
	if (retval)
		return retval;

	dev_dbg(up->port.dev, "serial_omap_startup+%d\n", up->port.line);

	/*
	 * Clear the FIFO buffers and disable them.
	 * (they will be reenabled in set_termios())
	 */
	serial_omap_clear_fifos(up);

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

	up->msr_saved_flags = 0;
	/*
	 * Finally, enable interrupts. Note: Modem status interrupts
	 * are set via set_termios(), which will be occurring imminently
	 * anyway, so we don't enable them here.
	 */
	up->ier = UART_IER_RLSI | UART_IER_RDI;
	serial_out(up, UART_IER, up->ier);

	/* Enable module level wake up */
	up->wer = OMAP_UART_WER_MOD_WKUP;
	if (up->features & OMAP_UART_WER_HAS_TX_WAKEUP)
		up->wer |= OMAP_UART_TX_WAKEUP_EN;

	serial_out(up, UART_OMAP_WER, up->wer);

	up->port_activity = jiffies;
	return 0;
}

static void serial_omap_shutdown(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	unsigned long flags = 0;

	FUNC_ENTER();

	dev_dbg(up->port.dev, "serial_omap_shutdown+%d\n", up->port.line);

	/*
	 * Disable interrupts from this port
	 */
	up->ier = 0;
	serial_out(up, UART_IER, 0);

	spin_lock_irqsave(&up->port.lock, flags);
	up->port.mctrl &= ~TIOCM_OUT2;
	serial_omap_set_mctrl(&up->port, up->port.mctrl);
	spin_unlock_irqrestore(&up->port.lock, flags);

	/*
	 * Disable break condition and FIFOs
	 */
	serial_out(up, UART_LCR, serial_in(up, UART_LCR) & ~UART_LCR_SBC);
	serial_omap_clear_fifos(up);

	/*
	 * Read data port to reset things, and then free the irq
	 */
	if (serial_in(up, UART_LSR) & UART_LSR_DR)
		(void) serial_in(up, UART_RX);

	free_irq(up->port.irq, up);
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
	quot = serial_omap_get_divisor(port, baud);

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

	/*
	 * Update the per-port timeout.
	 */
	uart_update_timeout(port, termios->c_cflag, baud);

	up->port.read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		up->port.read_status_mask |= UART_LSR_FE | UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		up->port.read_status_mask |= UART_LSR_BI;

	/*
	 * Characters to ignore
	 */
	up->port.ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		up->port.ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		up->port.ignore_status_mask |= UART_LSR_BI;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			up->port.ignore_status_mask |= UART_LSR_OE;
	}

	/*
	 * ignore all characters if CREAD is not set
	 */
	if ((termios->c_cflag & CREAD) == 0)
		up->port.ignore_status_mask |= UART_LSR_DR;
#if 0
	/*
	 * Modem status interrupts
	 */
	up->ier &= ~UART_IER_MSI;
	if (UART_ENABLE_MS(&up->port, termios->c_cflag))
		up->ier |= UART_IER_MSI;
#endif
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

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	up->efr = serial_in(up, UART_EFR) & ~UART_EFR_ECB;
	up->efr &= ~UART_EFR_SCD;
	serial_out(up, UART_EFR, up->efr | UART_EFR_ECB);

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	up->mcr = serial_in(up, UART_MCR) & ~UART_MCR_TCRTLR;
	serial_out(up, UART_MCR, up->mcr | UART_MCR_TCRTLR);
	/* FIFO ENABLE, DMA MODE */

	up->scr |= OMAP_UART_SCR_RX_TRIG_GRANU1_MASK;
	/*
	 * NOTE: Setting OMAP_UART_SCR_RX_TRIG_GRANU1_MASK
	 * sets Enables the granularity of 1 for TRIGGER RX
	 * level. Along with setting RX FIFO trigger level
	 * to 1 (as noted below, 16 characters) and TLR[3:0]
	 * to zero this will result RX FIFO threshold level
	 * to 1 character, instead of 16 as noted in comment
	 * below.
	 */

	/* Set receive FIFO threshold to 16 characters and
	 * transmit FIFO threshold to 16 spaces
	 */
	up->fcr &= ~OMAP_UART_FCR_RX_FIFO_TRIG_MASK;
	up->fcr &= ~OMAP_UART_FCR_TX_FIFO_TRIG_MASK;
	up->fcr |= UART_FCR6_R_TRIGGER_16 | UART_FCR6_T_TRIGGER_24 |
		UART_FCR_ENABLE_FIFO;

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

	if (!serial_omap_baud_is_mode16(port, baud))
		up->mdr1 = UART_OMAP_MDR1_13X_MODE;
	else
		up->mdr1 = UART_OMAP_MDR1_16X_MODE;
		serial_out(up, UART_OMAP_MDR1, up->mdr1);

	/* Configure flow control */
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	/* Enable access to TCR/TLR */
	serial_out(up, UART_EFR, up->efr | UART_EFR_ECB);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_out(up, UART_MCR, up->mcr | UART_MCR_TCRTLR);

	serial_out(up, UART_TI752_TCR, OMAP_UART_TCR_TRIG);
	serial_out(up, UART_MCR, up->mcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, up->lcr);

	serial_omap_set_mctrl(&up->port, up->port.mctrl);

	spin_unlock_irqrestore(&up->port.lock, flags);
	dev_dbg(up->port.dev, "serial_omap_set_termios+%d\n", up->port.line);
}

static void serial_omap_release_port(struct uart_port *port)
{
	FUNC_ENTER();
	dev_dbg(port->dev, "serial_omap_release_port+\n");
}

static int serial_omap_request_port(struct uart_port *port)
{
	FUNC_ENTER();
	dev_dbg(port->dev, "serial_omap_request_port+\n");
	return 0;
}

static const char *
serial_omap_type(struct uart_port *port)
{
	FUNC_ENTER();
	struct uart_omap_port *up = to_uart_omap_port(port);

	dev_dbg(up->port.dev, "serial_omap_type+%d\n", up->port.line);
	return up->name;
}

#define BOTH_EMPTY (UART_LSR_TEMT | UART_LSR_THRE)

static inline void wait_for_xmitr(struct uart_omap_port *up)
{
	unsigned int status, tmout = 10000;
	FUNC_ENTER();

	/* Wait up to 10ms for the character(s) to be sent. */
	do {
		status = serial_in(up, UART_LSR);

		if (status & UART_LSR_BI)
			up->lsr_break_flag = UART_LSR_BI;

		if (--tmout == 0)
			break;
		udelay(1);
	} while ((status & BOTH_EMPTY) != BOTH_EMPTY);

	/* Wait up to 1s for flow control if necessary */
	if (up->port.flags & UPF_CONS_FLOW) {
		tmout = 1000000;
		for (tmout = 1000000; tmout; tmout--) {
			unsigned int msr = serial_in(up, UART_MSR);

			up->msr_saved_flags |= msr & MSR_SAVE_FLAGS;
			if (msr & UART_MSR_CTS)
				break;

			udelay(1);
		}
	}
}
static void serial_omap_config_port(struct uart_port *port, int flags)
{
	        struct uart_omap_port *up = to_uart_omap_port(port);
	FUNC_ENTER();

		        dev_dbg(up->port.dev, "serial_omap_config_port+%d\n",
					                                                        up->port.line);
			        up->port.type = PORT_OMAP;
				        //up->port.flags |= UPF_SOFT_FLOW | UPF_HARD_FLOW;
					}

static int
serial_omap_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	        /* we don't want the core code to modify any port params */
	        dev_dbg(port->dev, "serial_omap_verify_port+\n");
		        return -EINVAL;
}

#ifdef CONFIG_CONSOLE_POLL

static void serial_omap_poll_put_char(struct uart_port *port, unsigned char ch)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	FUNC_ENTER();

	wait_for_xmitr(up);
	serial_out(up, UART_TX, ch);
}

static int serial_omap_poll_get_char(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	unsigned int status;
	FUNC_ENTER();

	status = serial_in(up, UART_LSR);
	if (!(status & UART_LSR_DR)) {
		status = NO_POLL_CHAR;
		goto out;
	}

	status = serial_in(up, UART_RX);

out:

	return status;
}

#endif /* CONFIG_CONSOLE_POLL */



static struct uart_ops serial_omap_pops = {
	.tx_empty	= serial_omap_tx_empty,
	.set_mctrl	= serial_omap_set_mctrl,
	.get_mctrl	= serial_omap_get_mctrl,
	.stop_tx	= serial_omap_stop_tx,
	.start_tx	= serial_omap_start_tx,
	.stop_rx	= serial_omap_stop_rx,
	.startup	= serial_omap_startup,
	.shutdown	= serial_omap_shutdown,
	.set_termios	= serial_omap_set_termios,
	.type		= serial_omap_type,
	.release_port	= serial_omap_release_port,
	.request_port	= serial_omap_request_port,
	.config_port	= serial_omap_config_port,
	.verify_port	= serial_omap_verify_port,
};

static struct uart_driver serial_omap_reg = {
	.owner		= THIS_MODULE,
	.driver_name	= "MY-SERIAL",
	.dev_name	= "ttym",
	.nr		= 1,
	.cons		= NULL,
};

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
	struct resource		*mem, *irq;
	struct omap_uart_port_info *omap_up_info = dev_get_platdata(&pdev->dev);
	int ret;

	if (pdev->dev.of_node) {
		omap_up_info = of_get_uart_port_info(&pdev->dev);
		pdev->dev.platform_data = omap_up_info;
	}
	printk("Here\n");
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq) {
		dev_err(&pdev->dev, "no irq resource?\n");
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
	up->DTR_active = 0;
	printk("Here 1\n");

	up->dev = &pdev->dev;
	up->port.dev = &pdev->dev;
	up->port.type = PORT_16550A;
	up->port.iotype = UPIO_MEM;
	up->port.irq = irq->start;

	up->port.regshift = 2;
	up->port.fifosize = 64;
	up->port.ops = &serial_omap_pops;

	printk("Here 2\n");
	if (pdev->dev.of_node)
		up->port.line = of_alias_get_id(pdev->dev.of_node, "serial");
	else
		up->port.line = pdev->id;

	if (up->port.line < 0) {
		dev_err(&pdev->dev, "failed to get alias/pdev id, errno %d\n",
								up->port.line);
		ret = -ENODEV;
		goto err_port_line;
	}
	up->port.line = 0;

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

	ui[up->port.line] = up;

	ret = uart_add_one_port(&serial_omap_reg, &up->port);
	if (ret != 0)
		goto err_add_port;

	printk("Here 7\n");
	return 0;

err_add_port:
err_ioremap:
err_port_line:
	dev_err(&pdev->dev, "[UART%d]: failure [%s]: %d\n",
				pdev->id, __func__, ret);
	return ret;
}

static int serial_omap_remove(struct platform_device *dev)
{
	struct uart_omap_port *up = platform_get_drvdata(dev);

	uart_remove_one_port(&serial_omap_reg, &up->port);

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
		.pm	= NULL, //&serial_omap_dev_pm_ops,
		.of_match_table = of_match_ptr(omap_serial_of_match),
	},
};

static int __init serial_omap_init(void)
{
	int ret;

	ret = uart_register_driver(&serial_omap_reg);
	if (ret != 0)
		return ret;
	ret = platform_driver_register(&serial_omap_driver);
	if (ret != 0)
		uart_unregister_driver(&serial_omap_reg);
	return ret;
}

static void __exit serial_omap_exit(void)
{
	platform_driver_unregister(&serial_omap_driver);
	uart_unregister_driver(&serial_omap_reg);
}

module_init(serial_omap_init);
module_exit(serial_omap_exit);

MODULE_DESCRIPTION("OMAP High Speed UART driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Texas Instruments Inc");
