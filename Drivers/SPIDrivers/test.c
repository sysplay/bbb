/*
 * OMAP2 McSPI controller driver
 *
 * Copyright (C) 2005, 2006 Nokia Corporation
 * Author:	Samuel Ortiz <samuel.ortiz@nokia.com> and
 *		Juha Yrj�l� <juha.yrjola@nokia.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/omap-dma.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gcd.h>
#include "spi_char.h"

#include <linux/spi/spi.h>

#include <linux/platform_data/spi-omap2-mcspi.h>

#define OMAP2_MCSPI_MAX_FREQ		48000000
#define OMAP2_MCSPI_MAX_FIFODEPTH	64
#define OMAP2_MCSPI_MAX_FIFOWCNT	0xFFFF
#define SPI_AUTOSUSPEND_TIMEOUT		2000

#define OMAP2_MCSPI_REVISION		0x00
#define OMAP2_MCSPI_SYSCONFIG		0x10
#define OMAP2_MCSPI_SYSSTATUS		0x14
#define OMAP2_MCSPI_IRQSTATUS		0x18
#define OMAP2_MCSPI_IRQENABLE		0x1c
#define OMAP2_MCSPI_WAKEUPENABLE	0x20
#define OMAP2_MCSPI_SYST		0x24
#define OMAP2_MCSPI_MODULCTRL		0x28
#define OMAP2_MCSPI_XFERLEVEL		0x7c
#define OMAP_MCSPI_SYS_RESET		0x02
#define SYSS_RESETDONE_MASK		0x01
/* per-channel banks, 0x14 bytes each, first is: */
#define OMAP2_MCSPI_CHCONF0		0x2c
#define OMAP2_MCSPI_CHSTAT0		0x30
#define OMAP2_MCSPI_CHCTRL0		0x34
#define OMAP2_MCSPI_TX0			0x38
#define OMAP2_MCSPI_RX0			0x3c

/* per-register bitmasks: */
#define OMAP2_MCSPI_IRQSTATUS_EOW	BIT(17)

#define OMAP2_MCSPI_MODULCTRL_SINGLE	BIT(0)
#define OMAP2_MCSPI_MODULCTRL_MS	BIT(2)
#define OMAP2_MCSPI_MODULCTRL_STEST	BIT(3)

#define OMAP2_MCSPI_CHCONF_PHA		BIT(0)
#define OMAP2_MCSPI_CHCONF_POL		BIT(1)
#define OMAP2_MCSPI_CHCONF_CLKD_MASK	(0x0f << 2)
#define OMAP2_MCSPI_CHCONF_EPOL		BIT(6)
#define OMAP2_MCSPI_CHCONF_WL_MASK	(0x1f << 7)
#define OMAP2_MCSPI_CHCONF_TRM_RX_ONLY	BIT(12)
#define OMAP2_MCSPI_CHCONF_TRM_TX_ONLY	BIT(13)
#define OMAP2_MCSPI_CHCONF_TRM_MASK	(0x03 << 12)
#define OMAP2_MCSPI_CHCONF_DMAW		BIT(14)
#define OMAP2_MCSPI_CHCONF_DMAR		BIT(15)
#define OMAP2_MCSPI_CHCONF_DPE0		BIT(16)
#define OMAP2_MCSPI_CHCONF_DPE1		BIT(17)
#define OMAP2_MCSPI_CHCONF_IS		BIT(18)
#define OMAP2_MCSPI_CHCONF_TURBO	BIT(19)
#define OMAP2_MCSPI_CHCONF_FORCE	BIT(20)
#define OMAP2_MCSPI_CHCONF_FFET		BIT(27)
#define OMAP2_MCSPI_CHCONF_FFER		BIT(28)

#define OMAP2_MCSPI_CHSTAT_RXS		BIT(0)
#define OMAP2_MCSPI_CHSTAT_TXS		BIT(1)
#define OMAP2_MCSPI_CHSTAT_EOT		BIT(2)
#define OMAP2_MCSPI_CHSTAT_TXFFE	BIT(3)

#define OMAP2_MCSPI_CHCTRL_EN		BIT(0)

#define OMAP2_MCSPI_WAKEUPENABLE_WKEN	BIT(0)

#define OMAP_SPI_TIMEOUT (msecs_to_jiffies(1000))

#define FUNC_ENTER() do { printk(KERN_INFO "Enter: %s\n", __func__); } while (0)

static inline void mcspi_write_reg(struct omap2_mcspi *mcspi,
		int idx, u32 val)
{
	__raw_writel(val, mcspi->base + idx);
}

static inline u32 mcspi_read_reg(struct omap2_mcspi *mcspi, int idx)
{
	return __raw_readl(mcspi->base + idx);
}

static inline u32 mcspi_cached_chconf0(struct omap2_mcspi *mcspi)
{
	return mcspi->chconf0;
}
static inline void mcspi_write_chconf0(struct omap2_mcspi *mcspi, u32 val)
{
	mcspi->chconf0 = val;
	mcspi_write_reg(mcspi, OMAP2_MCSPI_CHCONF0, val);
	mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCONF0);
}

static inline int mcspi_bytes_per_word(int word_len)
{
	if (word_len <= 8)
		return 1;
	else if (word_len <= 16)
		return 2;
	else /* word_len <= 32 */
		return 4;
}

static void omap2_mcspi_set_enable(struct omap2_mcspi *mcspi, int enable)
{
	u32 l;
	FUNC_ENTER();

	l = enable ? OMAP2_MCSPI_CHCTRL_EN : 0;
	mcspi_write_reg(mcspi, OMAP2_MCSPI_CHCTRL0, l);
	/* Flash post-writes */
	mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCTRL0);
}

static void omap2_mcspi_force_cs(struct omap2_mcspi *mcspi, int cs_active)
{
	u32 l;
	FUNC_ENTER();

	l = mcspi_cached_chconf0(mcspi);
	if (cs_active)
		l |= OMAP2_MCSPI_CHCONF_FORCE;
	else
		l &= ~OMAP2_MCSPI_CHCONF_FORCE;

	mcspi_write_chconf0(mcspi, l);
}

static void omap2_mcspi_set_master_mode(struct omap2_mcspi *mcspi)
{
	struct omap2_mcspi_regs	*ctx = &mcspi->ctx;
	u32 l;
	FUNC_ENTER();

	/*
	 * Setup when switching from (reset default) slave mode
	 * to single-channel master mode
	 */
	l = mcspi_read_reg(mcspi, OMAP2_MCSPI_MODULCTRL);
	l &= ~(OMAP2_MCSPI_MODULCTRL_STEST | OMAP2_MCSPI_MODULCTRL_MS);
	l |= OMAP2_MCSPI_MODULCTRL_SINGLE;
	mcspi_write_reg(mcspi, OMAP2_MCSPI_MODULCTRL, l);

	ctx->modulctrl = l;
}

static int mcspi_wait_for_reg_bit(void __iomem *reg, unsigned long bit)
{
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(1000);
	while (!(__raw_readl(reg) & bit)) {
		if (time_after(jiffies, timeout)) {
			if (!(__raw_readl(reg) & bit))
				return -ETIMEDOUT;
			else
				return 0;
		}
		cpu_relax();
	}
	return 0;
}

static unsigned
omap2_mcspi_txrx_pio(struct omap2_mcspi *mcspi, struct spi_transfer *xfer)
{
	unsigned int		count, c;
	void __iomem		*base = mcspi->base;
	void __iomem		*tx_reg;
	void __iomem		*rx_reg;
	void __iomem		*chstat_reg;
	int			word_len;
	u8		*rx;
	const u8	*tx;
	FUNC_ENTER();

	count = xfer->len;
	c = count;
	word_len = mcspi->word_len;

	/* We store the pre-calculated register addresses on stack to speed
	 * up the transfer loop. */
	tx_reg		= base + OMAP2_MCSPI_TX0;
	rx_reg		= base + OMAP2_MCSPI_RX0;
	chstat_reg	= base + OMAP2_MCSPI_CHSTAT0;

	if (c < (word_len>>3))
		return 0;

	rx = xfer->rx_buf;
	tx = xfer->tx_buf;

	do {
		c -= 1;
		if (tx != NULL) {
			if (mcspi_wait_for_reg_bit(chstat_reg,
						OMAP2_MCSPI_CHSTAT_TXS) < 0) {
				dev_err(mcspi->dev, "TXS timed out\n");
				goto out;
			}
			dev_vdbg(mcspi->dev, "write-%d %02x\n",
					word_len, *tx);
			__raw_writel(*tx++, tx_reg);
		}
		if (rx != NULL) {
			if (mcspi_wait_for_reg_bit(chstat_reg,
						OMAP2_MCSPI_CHSTAT_RXS) < 0) {
				dev_err(mcspi->dev, "RXS timed out\n");
				goto out;
			}
			*rx++ = __raw_readl(rx_reg);
			dev_vdbg(mcspi->dev, "read-%d %02x\n",
					word_len, *(rx - 1));
		}
	} while (c);
out:
	return count - c;
}

static u32 omap2_mcspi_calc_divisor(u32 speed_hz)
{
	u32 div;
	FUNC_ENTER();

	for (div = 0; div < 15; div++)
		if (speed_hz >= (OMAP2_MCSPI_MAX_FREQ >> div))
			return div;
	return 15;
}

/* called only when no transfer is active to this device */
int omap2_mcspi_setup_transfer(struct omap2_mcspi *mcspi,
		struct spi_transfer *t)
{
	u32 l = 0, div = 0;
	u32 speed_hz = 500000;
	u8 word_len = 8; //spi->bits_per_word;
	FUNC_ENTER();

	speed_hz = min_t(u32, speed_hz, OMAP2_MCSPI_MAX_FREQ);
	div = omap2_mcspi_calc_divisor(speed_hz);

	l = mcspi_cached_chconf0(mcspi);

	/* standard 4-wire master mode:  SCK, MOSI/out, MISO/in, nCS
	 * REVISIT: this controller could support SPI_3WIRE mode.
	 */
	if (mcspi->pin_dir == MCSPI_PINDIR_D0_IN_D1_OUT) {
		l &= ~OMAP2_MCSPI_CHCONF_IS;
		l &= ~OMAP2_MCSPI_CHCONF_DPE1;
		l |= OMAP2_MCSPI_CHCONF_DPE0;
	} else {
		l |= OMAP2_MCSPI_CHCONF_IS;
		l |= OMAP2_MCSPI_CHCONF_DPE1;
		l &= ~OMAP2_MCSPI_CHCONF_DPE0;
	}

	/* wordlength */
	l &= ~OMAP2_MCSPI_CHCONF_WL_MASK;
	l |= (word_len - 1) << 7;

	l &= ~OMAP2_MCSPI_CHCONF_EPOL;
	/* set clock divisor */
	l &= ~OMAP2_MCSPI_CHCONF_CLKD_MASK;
	l |= div << 2;
	l &= ~OMAP2_MCSPI_CHCONF_PHA;
	l &= ~OMAP2_MCSPI_CHCONF_POL;

	mcspi_write_chconf0(mcspi, l);
	omap2_mcspi_force_cs(mcspi, 1);

	return 0;
}

int spi_rw(struct omap2_mcspi *mcspi, char *buff)
{
	void __iomem		*base = mcspi->base;
	void __iomem		*tx_reg;
	void __iomem		*rx_reg;
	void __iomem		*chstat_reg;

	/* We store the pre-calculated register addresses on stack to speed
	 * up the transfer loop. */
	tx_reg		= base + OMAP2_MCSPI_TX0;
	rx_reg		= base + OMAP2_MCSPI_RX0;
	chstat_reg	= base + OMAP2_MCSPI_CHSTAT0;
	//Enable the Channel
	//Wait for TXS bit to be set
	//Write the data
	//Wait for RXS bit to be set
	//Read the data
	return 1;
}

int omap2_mcspi_transfer_one_message(struct omap2_mcspi *mcspi, 
						struct spi_transfer *t)
{
	int status = 0;
	FUNC_ENTER();
	
	if (t == NULL)
		return EINVAL;

	if (t->len) {
		unsigned	count;

		omap2_mcspi_set_enable(mcspi, 1);

		/* RX_ONLY mode needs dummy data in TX reg */
		if (t->tx_buf == NULL)
			__raw_writel(0, mcspi->base
					+ OMAP2_MCSPI_TX0);
		count = omap2_mcspi_txrx_pio(mcspi, t);

		if (count != t->len) {
			return -EIO;
		}
		omap2_mcspi_set_enable(mcspi, 0);
	}
	return status;
}

static struct omap2_mcspi_platform_config omap2_pdata = {
	.regs_offset = 0,
};

static struct omap2_mcspi_platform_config omap4_pdata = {
	.regs_offset = OMAP4_MCSPI_REG_OFFSET,
};

static const struct of_device_id omap_mcspi_of_match[] = {
	{
		.compatible = "ti,omap2-mcspi",
		.data = &omap2_pdata,
	},
	{
		.compatible = "ti,omap4-mcspi",
		.data = &omap4_pdata,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, omap_mcspi_of_match);

static int omap2_mcspi_probe(struct platform_device *pdev)
{
	const struct omap2_mcspi_platform_config *pdata;
	struct omap2_mcspi	*mcspi;
	struct resource		*r;
	int			status = 0;
	u32			regs_offset = 0;
	const struct of_device_id *match;

	mcspi = devm_kzalloc(&pdev->dev, sizeof(*mcspi), GFP_KERNEL);
	platform_set_drvdata(pdev, mcspi);

	match = of_match_device(omap_mcspi_of_match, &pdev->dev);
	if (match) {
		pdata = match->data;
	} else {
		pdata = dev_get_platdata(&pdev->dev);
	}
	regs_offset = pdata->regs_offset;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		return -ENODEV;
	}

	r->start += regs_offset;
	r->end += regs_offset;
	mcspi->phys = r->start;

	mcspi->base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(mcspi->base)) {
		status = PTR_ERR(mcspi->base);
		return status;
	}

	mcspi->dev = &pdev->dev;

	omap2_mcspi_set_master_mode(mcspi);
	fcd_init(mcspi);
	
	return status;
}

static int omap2_mcspi_remove(struct platform_device *pdev)
{
	fcd_exit();

	return 0;
}

/* work with hotplug and coldplug */
MODULE_ALIAS("platform:omap2_mcspi");

static struct platform_driver omap2_mcspi_driver = {
	.driver = {
		.name =		"omap2_mcspi",
		.owner =	THIS_MODULE,
		.pm =		NULL,
		.of_match_table = omap_mcspi_of_match,
	},
	.probe =	omap2_mcspi_probe,
	.remove =	omap2_mcspi_remove,
};

module_platform_driver(omap2_mcspi_driver);
MODULE_LICENSE("GPL");
