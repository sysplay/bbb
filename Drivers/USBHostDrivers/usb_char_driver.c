#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

#define BBB_VENDOR_ID 0x1A0A
#define BBB_PRODUCT_ID 0xBADD

static struct usb_class_driver ucd;

static int bbb_open(struct inode *i, struct file *f)
{
	return 0;
}
static int bbb_close(struct inode *i, struct file *f)
{
	return 0;
}

static struct file_operations bbb_fops =
{
	.owner = THIS_MODULE,
	.open = bbb_open,
	.release = bbb_close,
};

static int bbb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int i;
	int retval;

	iface_desc = interface->cur_altsetting;

	printk(KERN_INFO "BBB USB i/f %d now probed: (%04X:%04X)\n",
			iface_desc->desc.bInterfaceNumber, id->idVendor, id->idProduct);
	printk(KERN_INFO "ID->bNumEndpoints: %02X\n",
			iface_desc->desc.bNumEndpoints);
	printk(KERN_INFO "ID->bInterfaceClass: %02X\n",
			iface_desc->desc.bInterfaceClass);

	for (i = 0; i < iface_desc->desc.bNumEndpoints; i++)
	{
		endpoint = &iface_desc->endpoint[i].desc;

		printk(KERN_INFO "ED[%d]->bEndpointAddress: 0x%02X\n", i,
				endpoint->bEndpointAddress);
		printk(KERN_INFO "ED[%d]->bmAttributes: 0x%02X\n", i,
				endpoint->bmAttributes);
		printk(KERN_INFO "ED[%d]->wMaxPacketSize: 0x%04X (%d)\n", i,
				endpoint->wMaxPacketSize, endpoint->wMaxPacketSize);
	}

//	if (interface->cur_altsetting->desc.bInterfaceNumber == 0)
	{
		ucd.name = "bbb%d";
		ucd.fops = &bbb_fops;
		retval = usb_register_dev(interface, &ucd);
		if (retval)
		{
			/* Something prevented us from registering this driver */
			printk(KERN_ERR "Not able to get a minor for this device.\n");
			return retval;
		}
		else
		{
			printk(KERN_INFO "Minor obtained: %d\n", interface->minor);
		}
	}

	return 0;
}

static void bbb_disconnect(struct usb_interface *interface)
{
//	if (interface->cur_altsetting->desc.bInterfaceNumber == 0)
	{
		printk(KERN_INFO "Releasing Minor: %d\n", interface->minor);
		usb_deregister_dev(interface, &ucd);
	}

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
	.name = "bbb_vert",
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
MODULE_DESCRIPTION("USB w/ Vertical Driver for BBB");
