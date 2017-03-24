#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

#include "gpio.h"

static dev_t first;
static struct cdev c_dev;
static struct class *cl;
static unsigned int gpio_number[] = {53, 54, 55, 56}; // GPIO numbers for User LED 0-3: GPIO1_21-24

static int gpio_open(struct inode *i, struct file *f)
{
	return 0;
}

static int gpio_close(struct inode *i, struct file *f)
{
	return 0;
}

static long gpio_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	LEDData d;

	if (copy_from_user(&d, (LEDData *)arg, sizeof(LEDData)))
	{
		return -EFAULT;
	}

	switch (cmd)
	{
		case BBB_LED_GET:
			d.val = gpio_get_value(gpio_number[d.num]);
			if (copy_to_user((LEDData *)arg, &d, sizeof(LEDData)))
			{
				return -EFAULT;
			}
			break;
		case BBB_LED_SET:
			gpio_set_value(gpio_number[d.num], d.val);
			break;
		case BBB_GPIO_GET:
			d.val = gpio_get_value(d.num);
			if (copy_to_user((LEDData *)arg, &d, sizeof(LEDData)))
			{
				return -EFAULT;
			}
			break;
		case BBB_GPIO_SET:
			gpio_set_value(d.num, d.val);
			break;
		default:
			return -EINVAL;
			break;
	}
	return 0;
}

static struct file_operations file_ops =
{
	.owner			= THIS_MODULE,
	.open			= gpio_open,
	.release		= gpio_close,
	.unlocked_ioctl	= gpio_ioctl
};

static int gpio_init(void)
{
	int ret;
	struct device *dev_ret;

	if ((ret = alloc_chrdev_region(&first, 0, 1, "gpo_leds")) < 0)
	{
		printk(KERN_ERR "Device number registration failed\n");
		return ret;
	}

	cdev_init(&c_dev, &file_ops);

	if ((ret = cdev_add(&c_dev, first, 1)) < 0)
	{
		printk(KERN_ERR "Character device structure addition failed\n");
		unregister_chrdev_region(first, 1);
		return ret;
	}

	if (IS_ERR(cl = class_create(THIS_MODULE, "gpo_leds")))
	{
		printk(KERN_ERR "Class creation failed\n");
		cdev_del(&c_dev);
		unregister_chrdev_region(first, 1);
		return PTR_ERR(cl);
	}

	if (IS_ERR(dev_ret = device_create(cl, NULL, first, NULL, "led%d", 0)))
	{
		printk(KERN_ERR "Device creation failed\n");
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(first, 1);
		return PTR_ERR(dev_ret);
	}

	printk(KERN_INFO "GPO LEDs Driver registered\n");

	return 0;
}

void gpio_exit(void)
{
	device_destroy(cl, first);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(first, 1);

	printk(KERN_INFO "GPO LEDs Driver unregistered\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("BBB GPO LEDs Driver");
