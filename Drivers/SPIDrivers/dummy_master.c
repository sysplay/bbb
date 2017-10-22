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

#include <linux/spi/spi.h>

struct dummy_master {
	struct spi_master	*master;
	/* Virtual base address of the controller */
	void __iomem		*base;
	unsigned long		phys;
	struct device		*dev;
};

static int dummy_spi_setup(struct spi_device *spi)
{
	printk("@@@ Dummy setup invoked @@@\n");

	return 0;
}

static void dummy_spi_cleanup(struct spi_device *spi)
{
	printk("@@@ Dummy clean up invoked @@@\n");
}
static int dummy_transfer_one_message(struct spi_master *master,
		struct spi_message *m)
{
	printk("@@@ Dummy transfer_one_message invoked @@@\n");
	m->status = 0;
	spi_finalize_current_message(master);
	return 0;
}

static const struct of_device_id dummy_of_match[] = {
	{
	},
	{ },
};
MODULE_DEVICE_TABLE(of, dummy_of_match);

static int dummy_spi_probe(struct platform_device *pdev)
{
	struct spi_master	*master;
	//const struct omap2_mcspi_platform_config *pdata;
	struct dummy_master	*mcspi;
	struct resource		*r;
	int			status = 0, i;
	u32			regs_offset = 0;
	static int		bus_num = 1;
	struct device_node	*node = pdev->dev.of_node;
	const struct of_device_id *match;

	printk("@@@ Dummy SPI Probe invoked @@@\n");
	/* Allocate the master structure, initialize the mode_bits, setup, transfer,
	 * cleanup */

	master->dev.of_node = node;

	platform_set_drvdata(pdev, master);

	mcspi = spi_master_get_devdata(master);
	mcspi->master = master;

	match = of_match_device(dummy_of_match, &pdev->dev);
	if (match) {
		u32 num_cs = 1; /* default number of chipselect */
		of_property_read_u32(node, "num-cs", &num_cs);
		master->num_chipselect = num_cs;
		master->bus_num = bus_num++;
	} else {
		dev_err(&pdev->dev, "No matching device found for spi master\n");
		return -ENODEV;
	}
	/*  Register the master */
	return status;
}

static int dummy_spi_remove(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma	*dma_channels;

	master = platform_get_drvdata(pdev);
	mcspi = spi_master_get_devdata(master);
	/* Unregister the master */

	return 0;
}


static struct platform_driver dummy_spi_driver = {
};

module_platform_driver(dummy_spi_driver);
MODULE_LICENSE("GPL");
