#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

#include "gpio.h"

#define BBB_VENDOR_ID 0x1A0A
#define BBB_PRODUCT_ID 0xBADD
#define EP_IN (USB_DIR_IN | 0x01)
#define EP_OUT 0x01

#define LED_GPIO_BASE 53 // GPIO numbers for User LED 0-3: GPIO1_21-24: 53-56

struct bbb_device
{
	int in_ep, out_ep;
	GPIOFData buf;
	struct usb_device *device;
	struct usb_class_driver ucd;
};

static struct usb_driver bbb_driver;

static int bbb_open(struct inode *i, struct file *f)
{
	struct usb_interface *interface;
	struct bbb_device *dev;

	interface = usb_find_interface(&bbb_driver, iminor(i));
	if (!interface)
	{
		printk(KERN_ERR "%s - error, can't find device for minor %d\n",
				__func__, iminor(i));
		return -ENODEV;
	}   

	if (!(dev = usb_get_intfdata(interface)))
	{
		return -ENODEV;
	}

	f->private_data = (void *)(dev);
	return 0;
}
static int bbb_close(struct inode *i, struct file *f)
{
	return 0;
}
static long bbb_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct bbb_device *dev = (struct bbb_device *)(f->private_data);
	int retval, tran_size;

	if (copy_from_user(&dev->buf.d, (GPIOData *)arg, sizeof(GPIOData)))
	{
		return -EFAULT;
	}

	switch (cmd)
	{
		case BBB_LED_GET:
			dev->buf.d.num += LED_GPIO_BASE;
			// FALLTHROUGH
		case BBB_GPIO_GET:
			dev->buf.cmd = _BBB_GPIO_GET;
			/* Send the data out of the bulk endpoint */
			retval = usb_bulk_msg(dev->device, usb_sndbulkpipe(dev->device, dev->out_ep),
					&dev->buf, sizeof(dev->buf), &tran_size, 10);
			if (retval < 0)
			{
				printk(KERN_ERR "Bulk request returned %d\n", retval);
				return retval;
			}
			/* Read the data in from the bulk endpoint */
			retval = usb_bulk_msg(dev->device, usb_rcvbulkpipe(dev->device, dev->in_ep),
					&dev->buf, sizeof(dev->buf), &tran_size, 20); // Min of 20 timeout seems to be enough
			if (retval < 0)
			{
				printk(KERN_ERR "Bulk response returned %d\n", retval);
				return retval;
			}
			printk(KERN_INFO "BNum: %d\n", dev->buf.d.num);
			if (cmd == BBB_LED_GET)
			{
				dev->buf.d.num -= LED_GPIO_BASE;
			}
			printk(KERN_INFO "ANum: %d\n", dev->buf.d.num);
			if (copy_to_user((int *)arg, &dev->buf.d, sizeof(GPIOData)))
			{
				return -EFAULT;
			}
			break;
		case BBB_LED_SET:
			dev->buf.d.num += LED_GPIO_BASE;
			// FALLTHROUGH
		case BBB_GPIO_SET:
			dev->buf.cmd = _BBB_GPIO_SET;
			/* Send the data out of the bulk endpoint */
			retval = usb_bulk_msg(dev->device, usb_sndbulkpipe(dev->device, dev->out_ep),
					&dev->buf, sizeof(dev->buf), &tran_size, 10);
			if (retval < 0)
			{
				printk(KERN_ERR "Bulk command returned %d\n", retval);
				return retval;
			}
			break;
		default:
			return -EINVAL;
			break;
	}
	return 0;
}

static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.open = bbb_open,
	.release = bbb_close,
	.unlocked_ioctl = bbb_ioctl,
};

static int bbb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct bbb_device *bbb_dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int i;
	int retval;

	iface_desc = interface->cur_altsetting;
	printk(KERN_INFO "BBB USB i/f %d now probed: (%04X:%04X)\n",
			iface_desc->desc.bInterfaceNumber,
			id->idVendor, id->idProduct);

	bbb_dev = (struct bbb_device *)(kzalloc(sizeof(struct bbb_device), GFP_KERNEL));
	if (!bbb_dev)
	{
		printk(KERN_ERR "Not able to get memory for data structure of this device.\n");
		retval = -ENOMEM;
		goto probe_err;
	}

	/* Set up the endpoint information related to GPIO */
	for (i = 0; i < iface_desc->desc.bNumEndpoints; i++)
	{
		endpoint = &iface_desc->endpoint[i].desc;

		if (endpoint->bEndpointAddress == EP_IN)
		{
			bbb_dev->in_ep = endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
		}
		if (endpoint->bEndpointAddress == EP_OUT)
		{
			bbb_dev->out_ep = endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
		}
	}

	bbb_dev->device = interface_to_usbdev(interface);

	bbb_dev->ucd.name = "bbb%d";
	bbb_dev->ucd.fops = &fops;
	retval = usb_register_dev(interface, &bbb_dev->ucd);
	if (retval)
	{
		/* Something prevented us from registering this driver */
		printk(KERN_ERR "Not able to get a minor for this device.\n");
		goto probe_err;
	}
	else
	{
		printk(KERN_INFO "Minor obtained: %d\n", interface->minor);
	}

	usb_set_intfdata(interface, bbb_dev);

	return 0;

probe_err:
	if (bbb_dev)
	{
		kfree(bbb_dev);
	}

	return retval;
}

static void bbb_disconnect(struct usb_interface *interface)
{
	struct bbb_device *bbb_dev;

	printk(KERN_INFO "Releasing Minor: %d\n", interface->minor);

	bbb_dev = (struct bbb_device *)(usb_get_intfdata(interface));
	usb_set_intfdata(interface, NULL);

	/* Give back our minor */
	usb_deregister_dev(interface, &bbb_dev->ucd);

	kfree(bbb_dev);

	printk(KERN_INFO "BBB USB i/f %d now disconnected\n",
			interface->cur_altsetting->desc.bInterfaceNumber);
}

/* Table of devices that work with this driver */
static struct usb_device_id bbb_table[] =
{
	{
		USB_DEVICE(BBB_VENDOR_ID, BBB_PRODUCT_ID)
	},
	{} /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, bbb_table);

static struct usb_driver bbb_driver =
{
	.name = "bbb_gpio",
	.probe = bbb_probe,
	.disconnect = bbb_disconnect,
	.id_table = bbb_table,
};

static int __init bbb_init(void)
{
	int result;

	/* Register this driver with the USB subsystem */
	if ((result = usb_register(&bbb_driver)))
	{
		printk(KERN_ERR "usb_register failed. Error number %d\n", result);
	}
	printk(KERN_INFO "BBB usb_registered\n");
	return result;
}

static void __exit bbb_exit(void)
{
	/* Deregister this driver with the USB subsystem */
	usb_deregister(&bbb_driver);
	printk(KERN_INFO "BBB usb_deregistered\n");
}

module_init(bbb_init);
module_exit(bbb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("USB Host Driver for BBB GPIO Device");
