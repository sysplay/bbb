/*
 * TI OMAP I2C master mode driver
 *
 * Copyright (C) 2003 MontaVista Software, Inc.
 * Copyright (C) 2005 Nokia Corporation
 * Copyright (C) 2004 - 2007 Texas Instruments.
 *
 * Originally written by MontaVista Software, Inc.
 * Additional contributions by:
 *	Tony Lindgren <tony@atomide.com>
 *	Imre Deak <imre.deak@nokia.com>
 *	Juha Yrjölä <juha.yrjola@solidboot.com>
 *	Syed Khasim <x0khasim@ti.com>
 *	Nishant Menon <nm@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/i2c-omap.h>
#include <linux/pm_runtime.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include "i2c_char.h"

/* timeout waiting for the controller to respond */
#define OMAP_I2C_TIMEOUT (msecs_to_jiffies(1000))

/* timeout for pm runtime autosuspend */
#define OMAP_I2C_PM_TIMEOUT		1000	/* ms */
static struct class *i2c_class;

inline void omap_i2c_write_reg(struct omap_i2c_dev *i2c_dev,
				      int reg, u16 val)
{
	__raw_writew(val, i2c_dev->base + i2c_dev->regs[reg]);
}

inline u16 omap_i2c_read_reg(struct omap_i2c_dev *i2c_dev, int reg)
{
	return __raw_readw(i2c_dev->base + i2c_dev->regs[reg]);
}

void __omap_i2c_init(struct omap_i2c_dev *dev)
{
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, 0);

	/* Setup clock prescaler to obtain approx 12MHz I2C module clock: */
	omap_i2c_write_reg(dev, OMAP_I2C_PSC_REG, dev->pscstate);

	/* SCL low and high time values */
	omap_i2c_write_reg(dev, OMAP_I2C_SCLL_REG, dev->scllstate);
	omap_i2c_write_reg(dev, OMAP_I2C_SCLH_REG, dev->sclhstate);
		omap_i2c_write_reg(dev, OMAP_I2C_WE_REG, dev->westate);

	/* Take the I2C module out of reset: */
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, OMAP_I2C_CON_EN);

	/*
	 * Don't write to this register if the IE state is 0 as it can
	 * cause deadlock.
	 */
	if (dev->iestate)
		omap_i2c_write_reg(dev, OMAP_I2C_IE_REG, dev->iestate);
}

int omap_i2c_reset(struct omap_i2c_dev *dev)
{
	unsigned long timeout;
	u16 sysc;

	sysc = omap_i2c_read_reg(dev, OMAP_I2C_SYSC_REG);

	/* Disable I2C controller before soft reset */
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG,
			omap_i2c_read_reg(dev, OMAP_I2C_CON_REG) &
			~(OMAP_I2C_CON_EN));

	omap_i2c_write_reg(dev, OMAP_I2C_SYSC_REG, SYSC_SOFTRESET_MASK);
	/* For some reason we need to set the EN bit before the
	 * reset done bit gets set. */
	timeout = jiffies + OMAP_I2C_TIMEOUT;
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, OMAP_I2C_CON_EN);
	while (!(omap_i2c_read_reg(dev, OMAP_I2C_SYSS_REG) &
				SYSS_RESETDONE_MASK)) {
		if (time_after(jiffies, timeout)) {
			dev_warn(dev->dev, "timeout waiting "
					"for controller reset\n");
			return -ETIMEDOUT;
		}
		msleep(1);
	}

	/* SYSC register is cleared by the reset; rewrite it */
	omap_i2c_write_reg(dev, OMAP_I2C_SYSC_REG, sysc);

	return 0;
}

int omap_i2c_init(struct omap_i2c_dev *dev)
{
	u16 psc = 0, scll = 0, sclh = 0;
	u16 fsscll = 0, fssclh = 0, hsscll = 0, hssclh = 0;
	unsigned long fclk_rate = 12000000;
	unsigned long internal_clk = 0;
	struct clk *fclk;

	dev->westate = OMAP_I2C_WE_ALL;

	/*
	 * HSI2C controller internal clk rate should be 19.2 Mhz for
	 * HS and for all modes on 2430. On 34xx we can use lower rate
	 * to get longer filter period for better noise suppression.
	 * The filter is iclk (fclk for HS) period.
	 */
	if (dev->speed > 400 ||
			dev->flags & OMAP_I2C_FLAG_FORCE_19200_INT_CLK)
		internal_clk = 19200;
	else if (dev->speed > 100)
		internal_clk = 9600;
	else
		internal_clk = 4000;
	fclk = clk_get(dev->dev, "fck");
	fclk_rate = clk_get_rate(fclk) / 1000;
	clk_put(fclk);

	/* Compute prescaler divisor */
	psc = fclk_rate / internal_clk;
	psc = psc - 1;

	/* If configured for High Speed */
	if (dev->speed > 400) {
		unsigned long scl;

		/* For first phase of HS mode */
		scl = internal_clk / 400;
		fsscll = scl - (scl / 3) - 7;
		fssclh = (scl / 3) - 5;

		/* For second phase of HS mode */
		scl = fclk_rate / dev->speed;
		hsscll = scl - (scl / 3) - 7;
		hssclh = (scl / 3) - 5;
	} else if (dev->speed > 100) {
		unsigned long scl;

		/* Fast mode */
		scl = internal_clk / dev->speed;
		fsscll = scl - (scl / 3) - 7;
		fssclh = (scl / 3) - 5;
	} else {
		/* Standard mode */
		fsscll = internal_clk / (dev->speed * 2) - 7;
		fssclh = internal_clk / (dev->speed * 2) - 5;
	}
	scll = (hsscll << OMAP_I2C_SCLL_HSSCLL) | fsscll;
	sclh = (hssclh << OMAP_I2C_SCLH_HSSCLH) | fssclh;

	dev->iestate = (OMAP_I2C_IE_XRDY | OMAP_I2C_IE_RRDY |
			OMAP_I2C_IE_ARDY | OMAP_I2C_IE_NACK |
			OMAP_I2C_IE_AL)  | ((dev->fifo_size) ?
				(OMAP_I2C_IE_RDR | OMAP_I2C_IE_XDR) : 0);

	dev->pscstate = psc;
	dev->scllstate = scll;
	dev->sclhstate = sclh;

	__omap_i2c_init(dev);

	return 0;
}

/*
 * Waiting on Bus Busy
 */
int omap_i2c_wait_for_bb(struct omap_i2c_dev *dev)
{
	unsigned long timeout;

	timeout = jiffies + OMAP_I2C_TIMEOUT;
	while (omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG) & OMAP_I2C_STAT_BB) {
		if (time_after(jiffies, timeout)) {
			dev_warn(dev->dev, "timeout waiting for bus ready\n");
			return -ETIMEDOUT;
		}
		msleep(1);
	}

	return 0;
}

void omap_i2c_resize_fifo(struct omap_i2c_dev *dev, u8 size, bool is_rx)
{
	u16		buf;

	if (dev->flags & OMAP_I2C_FLAG_NO_FIFO)
		return;

	/*
	 * Set up notification threshold based on message size. We're doing
	 * this to try and avoid draining feature as much as possible. Whenever
	 * we have big messages to transfer (bigger than our total fifo size)
	 * then we might use draining feature to transfer the remaining bytes.
	 */

	dev->threshold = clamp(size, (u8) 1, dev->fifo_size);
	//printk("thr = %d\n", dev->threshold);

	buf = omap_i2c_read_reg(dev, OMAP_I2C_BUF_REG);

	if (is_rx) {
		/* Clear RX Threshold */
		buf &= ~(0x3f << 8);
		buf |= ((dev->threshold - 1) << 8) | OMAP_I2C_BUF_RXFIF_CLR;
	} else {
		/* Clear TX Threshold */
		buf &= ~0x3f;
		buf |= (dev->threshold - 1) | OMAP_I2C_BUF_TXFIF_CLR;
	}

	omap_i2c_write_reg(dev, OMAP_I2C_BUF_REG, buf);
}

/*
 * Low level master read/write transaction.
 */
static int omap_i2c_xfer_msg(struct i2c_adapter *adap,
			     struct i2c_msg *msg, int stop)
{
	struct omap_i2c_dev *dev = i2c_get_adapdata(adap);
	unsigned long timeout;
	u16 w;

	//printk("addr: 0x%04x, len: %d, flags: 0x%x, stop: %d\n",
		//msg->addr, msg->len, msg->flags, stop);

	if (msg->len == 0)
		return -EINVAL;

	dev->receiver = !!(msg->flags & I2C_M_RD);
	omap_i2c_resize_fifo(dev, msg->len, dev->receiver);

	omap_i2c_write_reg(dev, OMAP_I2C_SA_REG, msg->addr);

	/* REVISIT: Could the STB bit of I2C_CON be used with probing? */
	dev->buf = msg->buf;
	dev->buf_len = msg->len;

	/* make sure writes to dev->buf_len are ordered */
	barrier();

	omap_i2c_write_reg(dev, OMAP_I2C_CNT_REG, dev->buf_len);

	/* Clear the FIFO Buffers */
	w = omap_i2c_read_reg(dev, OMAP_I2C_BUF_REG);
	w |= OMAP_I2C_BUF_RXFIF_CLR | OMAP_I2C_BUF_TXFIF_CLR;
	omap_i2c_write_reg(dev, OMAP_I2C_BUF_REG, w);
	w = omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG);
	//printk("Stat reg = 0x%04x\n", w);

	INIT_COMPLETION(dev->cmd_complete);
	dev->cmd_err = 0;

	w = OMAP_I2C_CON_EN | OMAP_I2C_CON_MST | OMAP_I2C_CON_STT;

	/* High speed configuration */
	if (dev->speed > 400)
		w |= OMAP_I2C_CON_OPMODE_HS;

	if (msg->flags & I2C_M_STOP)
		stop = 1;
	if (!(msg->flags & I2C_M_RD))
		w |= OMAP_I2C_CON_TRX;

		w |= OMAP_I2C_CON_STP;

	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, w);

	/*
	 * REVISIT: We should abort the transfer on signals, but the bus goes
	 * into arbitration and we're currently unable to recover from it.
	 */
	timeout = wait_for_completion_timeout(&dev->cmd_complete,
						OMAP_I2C_TIMEOUT);
	if (timeout == 0) {
		dev_err(dev->dev, "controller timed out\n");
		omap_i2c_reset(dev);
		__omap_i2c_init(dev);
		return -ETIMEDOUT;
	}

	if (likely(!dev->cmd_err))
		return 0;

	/* We have an error */
	if (dev->cmd_err & (OMAP_I2C_STAT_AL | OMAP_I2C_STAT_ROVR |
			    OMAP_I2C_STAT_XUDF)) {
		omap_i2c_reset(dev);
		__omap_i2c_init(dev);
		return -EIO;
	}

	if (dev->cmd_err & OMAP_I2C_STAT_NACK) {
		if (msg->flags & I2C_M_IGNORE_NAK)
			return 0;

		w = omap_i2c_read_reg(dev, OMAP_I2C_CON_REG);
		w |= OMAP_I2C_CON_STP;
		omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, w);
		return -EREMOTEIO;
	}
	return -EIO;
}


/*
 * Prepare controller for a transaction and call omap_i2c_xfer_msg
 * to do the work during IRQ processing.
 */
int omap_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct omap_i2c_dev *dev = i2c_get_adapdata(adap);
	int i;
	int r;

	r = omap_i2c_wait_for_bb(dev);
	if (r < 0)
	{
		printk("Error r = %d\n", r);
		goto out;
	}

	for (i = 0; i < num; i++) {
		r = omap_i2c_xfer_msg(adap, &msgs[i], (i == (num - 1)));
		if (r != 0)
			break;
	}

	if (r == 0)
		r = num;

	omap_i2c_wait_for_bb(dev);

out:
	return r;
}

static u32
omap_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK) |
	       I2C_FUNC_PROTOCOL_MANGLING;
}

static inline void
omap_i2c_complete_cmd(struct omap_i2c_dev *dev, u16 err)
{
	dev->cmd_err |= err;
	complete(&dev->cmd_complete);
}

inline void
omap_i2c_ack_stat(struct omap_i2c_dev *dev, u16 stat)
{
	omap_i2c_write_reg(dev, OMAP_I2C_STAT_REG, stat);
}


static void omap_i2c_receive_data(struct omap_i2c_dev *dev, u8 num_bytes,
		bool is_rdr)
{
	u16		w;

	while (num_bytes--) {
		w = omap_i2c_read_reg(dev, OMAP_I2C_DATA_REG);
		*dev->buf++ = w;
		dev->buf_len--;

		/*
		 * Data reg in 2430, omap3 and
		 * omap4 is 8 bit wide
		 */
		if (dev->flags & OMAP_I2C_FLAG_16BIT_DATA_REG) {
			*dev->buf++ = w >> 8;
			dev->buf_len--;
		}
	}
}

static int omap_i2c_transmit_data(struct omap_i2c_dev *dev, u8 num_bytes,
		bool is_xdr)
{
	u16		w;

	while (num_bytes--) {
		w = *dev->buf++;
		dev->buf_len--;

		/*
		 * Data reg in 2430, omap3 and
		 * omap4 is 8 bit wide
		 */
		if (dev->flags & OMAP_I2C_FLAG_16BIT_DATA_REG) {
			w |= *dev->buf++ << 8;
			dev->buf_len--;
		}

		omap_i2c_write_reg(dev, OMAP_I2C_DATA_REG, w);
	}

	return 0;
}

static irqreturn_t
omap_i2c_isr(int irq, void *dev_id)
{
	struct omap_i2c_dev *dev = dev_id;
	irqreturn_t ret = IRQ_HANDLED;
	u16 mask;
	u16 stat;

	spin_lock(&dev->lock);
	mask = omap_i2c_read_reg(dev, OMAP_I2C_IE_REG);
	stat = omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG);

	if (stat & mask)
		ret = IRQ_WAKE_THREAD;

	spin_unlock(&dev->lock);

	return ret;
}

static irqreturn_t
omap_i2c_isr_thread(int this_irq, void *dev_id)
{
	struct omap_i2c_dev *dev = dev_id;
	unsigned long flags;
	u16 bits;
	u16 stat;
	int err = 0, count = 0;

	spin_lock_irqsave(&dev->lock, flags);
	do {
		bits = omap_i2c_read_reg(dev, OMAP_I2C_IE_REG);
		stat = omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG);
		stat &= bits;

		/* If we're in receiver mode, ignore XDR/XRDY */
		if (dev->receiver)
			stat &= ~(OMAP_I2C_STAT_XDR | OMAP_I2C_STAT_XRDY);
		else
			stat &= ~(OMAP_I2C_STAT_RDR | OMAP_I2C_STAT_RRDY);

		if (!stat) {
			/* my work here is done */
			goto out;
		}

		dev_dbg(dev->dev, "IRQ (ISR = 0x%04x)\n", stat);
		//printk("IRQ (ISR = 0x%04x\n", stat);
		if (count++ == 100) {
			dev_warn(dev->dev, "Too much work in one IRQ\n");
			break;
		}

		if (stat & OMAP_I2C_STAT_NACK) {
			err |= OMAP_I2C_STAT_NACK;
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_NACK);
			break;
		}

		if (stat & OMAP_I2C_STAT_AL) {
			dev_err(dev->dev, "Arbitration lost\n");
			err |= OMAP_I2C_STAT_AL;
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_AL);
			break;
		}

		/*
		 * ProDB0017052: Clear ARDY bit twice
		 */
		if (stat & OMAP_I2C_STAT_ARDY)
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_ARDY);

		if (stat & (OMAP_I2C_STAT_ARDY | OMAP_I2C_STAT_NACK |
					OMAP_I2C_STAT_AL)) {
			omap_i2c_ack_stat(dev, (OMAP_I2C_STAT_RRDY |
						OMAP_I2C_STAT_RDR |
						OMAP_I2C_STAT_XRDY |
						OMAP_I2C_STAT_XDR |
						OMAP_I2C_STAT_ARDY));
			break;
		}

		if (stat & OMAP_I2C_STAT_RDR) {
			u8 num_bytes = 1;

			if (dev->fifo_size)
				num_bytes = dev->buf_len;

			omap_i2c_receive_data(dev, num_bytes, true);

			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_RDR);
			continue;
		}

		if (stat & OMAP_I2C_STAT_RRDY) {
			u8 num_bytes = 1;

			if (dev->threshold)
				num_bytes = dev->threshold;

			omap_i2c_receive_data(dev, num_bytes, false);
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_RRDY);
			continue;
		}

		if (stat & OMAP_I2C_STAT_XDR) {
			u8 num_bytes = 1;
			int ret;

			if (dev->fifo_size)
				num_bytes = dev->buf_len;

			ret = omap_i2c_transmit_data(dev, num_bytes, true);
			if (ret < 0)
				break;

			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_XDR);
			continue;
		}

		if (stat & OMAP_I2C_STAT_XRDY) {
			u8 num_bytes = 1;
			int ret;

			if (dev->threshold)
				num_bytes = dev->threshold;

			ret = omap_i2c_transmit_data(dev, num_bytes, false);
			if (ret < 0)
				break;

			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_XRDY);
			continue;
		}

		if (stat & OMAP_I2C_STAT_ROVR) {
			//printk("Receive overrun\n");
			err |= OMAP_I2C_STAT_ROVR;
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_ROVR);
			break;
		}

		if (stat & OMAP_I2C_STAT_XUDF) {
			//printk("Transmit Underflow\n");
			err |= OMAP_I2C_STAT_XUDF;
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_XUDF);
			break;
		}
	} while (stat);

	omap_i2c_complete_cmd(dev, err);

out:
	spin_unlock_irqrestore(&dev->lock, flags);

	return IRQ_HANDLED;
}

static const struct i2c_algorithm omap_i2c_algo = {
	.master_xfer	= omap_i2c_xfer,
	.functionality	= omap_i2c_func,
};

#ifdef CONFIG_OF
static struct omap_i2c_bus_platform_data omap3_pdata = {
	.rev = OMAP_I2C_IP_VERSION_1,
	.flags = OMAP_I2C_FLAG_BUS_SHIFT_2,
};

static struct omap_i2c_bus_platform_data omap4_pdata = {
	.rev = OMAP_I2C_IP_VERSION_2,
};

static const struct of_device_id omap_i2c_of_match[] = {
	{
		.compatible = "ti,omap4-i2c",
		.data = &omap4_pdata,
	},
	{
		.compatible = "ti,omap3-i2c",
		.data = &omap3_pdata,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, omap_i2c_of_match);
#endif

static int
omap_i2c_probe(struct platform_device *pdev)
{
	struct omap_i2c_dev	*dev;
	struct i2c_adapter	*adap;
	struct resource		*mem;
	const struct omap_i2c_bus_platform_data *pdata =
		dev_get_platdata(&pdev->dev);
	struct device_node	*node = pdev->dev.of_node;
	const struct of_device_id *match;
	int irq;
	int r;
	u16 s;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return irq;
	}

	dev = devm_kzalloc(&pdev->dev, sizeof(struct omap_i2c_dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&pdev->dev, "Menory allocation failed\n");
		return -ENOMEM;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(dev->base))
		return PTR_ERR(dev->base);

	match = of_match_device(of_match_ptr(omap_i2c_of_match), &pdev->dev);
	if (match) {
		u32 freq = 100000; /* default to 100000 Hz */

		pdata = match->data;
		dev->flags = pdata->flags;

		of_property_read_u32(node, "clock-frequency", &freq);
		/* convert DT freq value in Hz into kHz for speed */
		dev->speed = freq / 1000;
	} else if (pdata != NULL) {
		dev->speed = pdata->clkrate;
		dev->flags = pdata->flags;
	}

	dev->dev = &pdev->dev;
	dev->irq = irq;

	spin_lock_init(&dev->lock);

	platform_set_drvdata(pdev, dev);
	init_completion(&dev->cmd_complete);

	dev->regs = (u8 *)reg_map_ip_v2;

	/* Set up the fifo size - Get total size */
	s = (omap_i2c_read_reg(dev, OMAP_I2C_BUFSTAT_REG) >> 14) & 0x3;
	dev->fifo_size = 0x8 << s;

	/*
	 * Set up notification threshold as half the total available
	 * size. This is to ensure that we can handle the status on int
	 * call back latencies.
	 */

	dev->fifo_size = (dev->fifo_size / 2);

	/* reset ASAP, clearing any IRQs */
	omap_i2c_init(dev);
	r = devm_request_threaded_irq(&pdev->dev, dev->irq,
			omap_i2c_isr, omap_i2c_isr_thread,
			IRQF_NO_SUSPEND | IRQF_ONESHOT,
			pdev->name, dev);

	if (r) {
		dev_err(dev->dev, "failure requesting irq %i\n", dev->irq);
		goto err_unuse_clocks;
	}

	adap = &dev->adapter;
	i2c_set_adapdata(adap, dev);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "OMAP I2C adapter", sizeof(adap->name));
	adap->algo = &omap_i2c_algo;
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;

	/* i2c device drivers may be active on return from add_adapter() */
	adap->nr = pdev->id;
	r = i2c_add_numbered_adapter(adap);
	if (r) {
		dev_err(dev->dev, "failure adding adapter\n");
		goto err_unuse_clocks;
	}

	/* Char interface related initialization */
//	dev->i2c_class = i2c_class;
//	fcd_init(dev);

	return 0;

err_unuse_clocks:
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, 0);
	return r;
}

static int omap_i2c_remove(struct platform_device *pdev)
{
	struct omap_i2c_dev *dev = platform_get_drvdata(pdev);

	//fcd_exit(dev);

	i2c_del_adapter(&dev->adapter);

	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, 0);
	return 0;
}

static struct platform_driver omap_i2c_driver = {
	.probe		= omap_i2c_probe,
	.remove		= omap_i2c_remove,
	.driver		= {
		.name	= "omap_i2c",
		.owner	= THIS_MODULE,
		.pm	= NULL,
		.of_match_table = of_match_ptr(omap_i2c_of_match),
	},
};
/* I2C may be needed to bring up other drivers */
static int __init
omap_i2c_init_driver(void)
{
	if ((i2c_class = class_create(THIS_MODULE, "i2cdrv")) == NULL)
	{
		printk( KERN_ALERT "Class creation failed\n" );
		return -1;
	}
	return platform_driver_register(&omap_i2c_driver);
}
module_init(omap_i2c_init_driver);

static void __exit omap_i2c_exit_driver(void)
{
	platform_driver_unregister(&omap_i2c_driver);
	class_destroy(i2c_class);
}
module_exit(omap_i2c_exit_driver);

MODULE_AUTHOR("MontaVista Software, Inc. (and others)");
MODULE_DESCRIPTION("TI OMAP I2C bus adapter");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:omap_i2c");
