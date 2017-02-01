#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/platform_data/serial-omap.h>
#include "spi_char.h"

#define FIRST_MINOR 0
#define MINOR_CNT 1

static uint8_t bits = 8;
static uint32_t speed = 500000;
static uint16_t delay = 0;
static struct omap2_mcspi *mcspi;

static int my_open(struct inode *i, struct file *f)
{
	return omap2_mcspi_setup_transfer(mcspi, NULL);
}
static int my_close(struct inode *i, struct file *f)
{
	return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	uint8_t tx[] = {0x01, 0x80, 0x00, 0x01, 0x80};
	uint8_t rx[] = {0x00, 0x00, 0x00, 0x00, 0x00};

	struct spi_transfer t = {
		.tx_buf = tx,
		.rx_buf = rx,
		.len = ARRAY_SIZE(tx),
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};
	omap2_mcspi_transfer_one_message(mcspi, &t);

	printk("%d\t %d\t %d\t", rx[0], rx[1], rx[2]);
	return 0;
}
static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	return len;
}

static struct file_operations driver_fops =
{
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.read = my_read,
	.write = my_write
};

int fcd_init(struct omap2_mcspi *lmcspi)
{
	int ret;
	struct device *dev_ret;
	mcspi = lmcspi;

	if ((ret = alloc_chrdev_region(&mcspi->devt, FIRST_MINOR, MINOR_CNT, "spi_driver")) < 0)
	{
		return ret;
	}

	cdev_init(&mcspi->cdev, &driver_fops);

	if ((ret = cdev_add(&mcspi->cdev, mcspi->devt, MINOR_CNT)) < 0)
	{
		unregister_chrdev_region(mcspi->devt, MINOR_CNT);
		return ret;
	}
	
	if (IS_ERR(lmcspi->spi_class = class_create(THIS_MODULE, "spi")))
	{
		cdev_del(&mcspi->cdev);
		unregister_chrdev_region(mcspi->devt, MINOR_CNT);
		return PTR_ERR(lmcspi->spi_class);
	}
	if (IS_ERR(dev_ret = device_create(mcspi->spi_class, NULL, mcspi->devt, NULL, "spi%d", FIRST_MINOR)))
	{
		class_destroy(mcspi->spi_class);
		cdev_del(&mcspi->cdev);
		unregister_chrdev_region(mcspi->devt, MINOR_CNT);
		return PTR_ERR(dev_ret);
	}

	return 0;
}

void fcd_exit(void)
{
	device_destroy(mcspi->spi_class, mcspi->devt);
	class_destroy(mcspi->spi_class);
	cdev_del(&mcspi->cdev);
	unregister_chrdev_region(mcspi->devt, MINOR_CNT);
}

