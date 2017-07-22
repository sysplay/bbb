/*
 * Copyright (C) 2013 Oskar Andero <oskar.andero@gmail.com>
 *
 * Driver for Microchip Technology's MCP3204 and MCP3208 ADC chips.
 * Datasheet can be found here:
 * http://ww1.microchip.com/downloads/en/devicedoc/21298c.pdf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>

struct dummy_data {
	struct spi_device *spi;
	struct spi_message msg;
	struct spi_transfer transfer[2];

	u8 tx_buf;
	u8 rx_buf[2];
	/* Character Driver Files */
	dev_t devt;
	struct cdev cdev;
	struct class *class;
};


static ssize_t dummy_read(struct file* f, char *buf, size_t count, loff_t *f_pos)
{
	struct dummy_data *dev = (struct dummy_data *)(f->private_data);
	int ret;

	if (*f_pos == 0) {
		dev->tx_buf = 3;
		ret = spi_sync(dev->spi, &dev->msg);
		if (ret < 0)
			return ret;

		if (copy_to_user(buf, dev->rx_buf, 2)) {
			printk("Failed to send to user space\n");
			return -EFAULT;
		}
		*f_pos = 1;
		return 2;
	} 
	else {
		*f_pos = 0;
		return 0;
	}

	return 0;
}

static int dummy_close(struct inode *i, struct file *file)
{
	return 0;
}

static int dummy_open(struct inode *i, struct file *f)
{
	//struct i2c_adapter *adap;
	struct dummy_data *dev = container_of(i->i_cdev, struct dummy_data, cdev);
	if (dev == NULL) {
		printk("Data is null\n");
		return -1;
	}
	f->private_data = dev;

	return 0;
}

struct file_operations fops = {
	.open = dummy_open,
	.release = dummy_close,
	.read = dummy_read,
};

static int dummy_probe(struct spi_device *spi)
{
	struct dummy_data *data;
	int ret, init_result;

	data = devm_kzalloc(&spi->dev, sizeof(struct dummy_data), GFP_KERNEL);
	data->spi = spi;

	data->transfer[0].tx_buf = &data->tx_buf;
	data->transfer[0].len = sizeof(data->tx_buf);
	data->transfer[1].rx_buf = data->rx_buf;
	data->transfer[1].len = sizeof(data->rx_buf);

	spi_message_init_with_transfers(&data->msg, data->transfer,
					ARRAY_SIZE(data->transfer));
	spi_set_drvdata(spi, data);

	init_result = alloc_chrdev_region(&data->devt, 0, 1, "spi_dmy");

	if (0 > init_result)
	{
		printk(KERN_ALERT "Device Registration failed\n");
		unregister_chrdev_region(data->devt, 1);
		return -1;
	}
	printk("Major Nr: %d\n", MAJOR(data->devt));

	if ((data->class = class_create(THIS_MODULE, "spidummy")) == NULL)
	{
		printk( KERN_ALERT "Class creation failed\n" );
		unregister_chrdev_region(data->devt, 1);
		return -1;
	}
	if (device_create(data->class, NULL, data->devt, NULL, "spi_dmy%d", 0) == NULL)
	{
		printk( KERN_ALERT "Device creation failed\n" );
		class_destroy(data->class);
		unregister_chrdev_region(data->devt, 1);
		return -1;
	}
	
	cdev_init(&data->cdev, &fops);

	if (cdev_add(&data->cdev, data->devt, 1) == -1)
	{
		printk( KERN_ALERT "Device addition failed\n" );
		device_destroy(data->class, data->devt);
		class_destroy(data->class);
		unregister_chrdev_region(data->devt, 1 );
		return -1;
	}
	return 0;
}

static int dummy_remove(struct spi_device *spi)
{
	return 0;
}

static const struct spi_device_id dummy_id[] = {
	{ "dummy-spi", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, dummy_id);

static struct spi_driver dummy_driver = {
	.driver = {
		.name = "dummy_client",
		.owner = THIS_MODULE,
	},
	.probe = dummy_probe,
	.remove = dummy_remove,
	.id_table = dummy_id,
};
module_spi_driver(dummy_driver);

MODULE_AUTHOR("Oskar Andero <oskar.andero@gmail.com>");
MODULE_DESCRIPTION("Microchip Technology MCP3204/08");
MODULE_LICENSE("GPL v2");
