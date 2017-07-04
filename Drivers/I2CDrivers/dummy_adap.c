/*
 *
 * Copyright (C) 2003 MontaVista Software, Inc.
 * Copyright (C) 2005 Nokia Corporation
 * Copyright (C) 2004 - 2007 Texas Instruments.
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
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>


static u32 dummy_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK) |
	       I2C_FUNC_PROTOCOL_MANGLING;
}

/*
 * Prepare controller for a transaction and call omap_i2c_xfer_msg
 * to do the work during IRQ processing.
 */
int dummy_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	printk("Dummy transfer invoked\n");
	return num;
}

static const struct i2c_algorithm dummy_i2c_algo = {
	.master_xfer	= dummy_i2c_xfer,
	.functionality	= dummy_i2c_func,
};

#ifdef CONFIG_OF

static const struct of_device_id dummy_i2c_of_match[] = {
	{
		.compatible = "dummy_adap",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, dummy_i2c_of_match);
#endif

static int
dummy_i2c_probe(struct platform_device *pdev)
{
	struct i2c_adapter	*adap;
	int r;

	printk("Dummy Adapter probe invoked\n");
	adap = devm_kzalloc(&pdev->dev, sizeof(struct i2c_adapter), GFP_KERNEL);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "Dummy I2C adapter", sizeof(adap->name));
	adap->algo = &dummy_i2c_algo;
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;

	/* i2c device drivers may be active on return from add_adapter() */
	adap->nr = pdev->id;
	r = i2c_add_numbered_adapter(adap);
	if (r) {
		printk("Failure adding adapter\n");
		return r;
	}
	platform_set_drvdata(pdev, adap);
	return r;
}

static int dummy_i2c_remove(struct platform_device *pdev)
{
	struct i2c_adapter *adapter = platform_get_drvdata(pdev);
	i2c_del_adapter(adapter);

	return 0;
}

static struct platform_driver dummy_i2c_driver = {
	.probe		= dummy_i2c_probe,
	.remove		= dummy_i2c_remove,
	.driver		= {
		.name	= "dummy_adap",
		.owner	= THIS_MODULE,
		.pm	= NULL,
		.of_match_table = of_match_ptr(dummy_i2c_of_match),
	},
};
/* I2C may be needed to bring up other drivers */
static int __init dummy_i2c_init_driver(void)
{
	return platform_driver_register(&dummy_i2c_driver);
}
module_init(dummy_i2c_init_driver);

static void __exit dummy_i2c_exit_driver(void)
{
	platform_driver_unregister(&dummy_i2c_driver);
}
module_exit(dummy_i2c_exit_driver);

MODULE_AUTHOR("Sysplay Workshops");
MODULE_DESCRIPTION("Dummy I2C bus adapter");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dummy_i2c");
