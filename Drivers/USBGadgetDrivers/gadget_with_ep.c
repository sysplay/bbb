#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/errno.h>

/* OTG test device IDs */
#define BBB_GEP_VID	0x1A0A
#define BBB_GEP_PID	0xBADD

#define EP_BULK

#ifndef EP_BULK
#define EP_XFER USB_ENDPOINT_XFER_INT
//#define EP_XFER USB_ENDPOINT_XFER_ISOC
#define EP_IVL 1
#define COMPUTE_EP_IVL(ep) (125 * (1 << ((ep)->desc->bInterval - 1))) // us
#else
#define EP_XFER USB_ENDPOINT_XFER_BULK
#define EP_IVL 0
#define COMPUTE_EP_IVL(ep) (125 * ((ep)->desc->bInterval)) // us
#endif

static struct usb_device_descriptor device_desc = {
	.bLength = sizeof(device_desc),
	.bDescriptorType = USB_DT_DEVICE,

	.bcdUSB = cpu_to_le16(0x0200),
	.bDeviceClass = USB_CLASS_VENDOR_SPEC,

	.idVendor = cpu_to_le16(BBB_GEP_VID),
	.idProduct = cpu_to_le16(BBB_GEP_PID),
	.bNumConfigurations = 1,
};

static struct usb_string strings[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "SysPlay eLearning Academy for You <workshop@sysplay.in>",
	[USB_GADGET_PRODUCT_IDX].s = "BBB GEP Device",
	[USB_GADGET_SERIAL_IDX].s = "1.00",
	{ } /* end of list */
};

static struct usb_gadget_strings strings_tab = {
	.language = 0x0409, /* en-us */
	.strings = strings,
};

static struct usb_gadget_strings *strings_desc[] = {
	&strings_tab,
	NULL,
};

static struct usb_configuration config = {
	.label = "gep_conf",
	.bConfigurationValue = 1,
	.bmAttributes = USB_CONFIG_ATT_SELFPOWER,
};

static struct usb_interface_descriptor intf_desc = {
	.bLength = sizeof(intf_desc),
	.bDescriptorType = USB_DT_INTERFACE,

	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
};

static struct usb_endpoint_descriptor ep_out_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = EP_XFER,
	.bInterval = EP_IVL,
};
static struct usb_endpoint_descriptor ep_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = EP_XFER,
	.bInterval = EP_IVL,
};

static struct usb_descriptor_header *intf_descs[] = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &ep_out_desc,
	(struct usb_descriptor_header *) &ep_in_desc,
	NULL,
};

static struct usb_ep *ep_in, *ep_out;

/*-------------------------------------------------------------------------*/

static int gep_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	int id, ret;

	printk(KERN_INFO "GEP %s bind\n", f->name);

	/* Allocate interface ID(s) */
	if ((id = usb_interface_id(c, f)) < 0)
		return id;
	intf_desc.bInterfaceNumber = id;
	printk(KERN_INFO "GEP i/f id: %d\n", id);

	if (!(ep_out = usb_ep_autoconfig(cdev->gadget, &ep_out_desc)))
	{
		printk(KERN_ERR "GEP %s: Can't autoconfigure ep_out on %s\n", f->name, cdev->gadget->name);
		return -ENODEV;
	}
	ep_out->driver_data = cdev; /* Claim */
	if (!(ep_in = usb_ep_autoconfig(cdev->gadget, &ep_in_desc)))
	{
		printk(KERN_ERR "GEP %s: Can't autoconfigure ep_in on %s\n", f->name, cdev->gadget->name);
		ep_out = NULL;
		usb_ep_autoconfig_reset(cdev->gadget);
		return -ENODEV;
	}
	ep_in->driver_data = cdev; /* Claim */

	if ((ret = usb_assign_descriptors(f, NULL, intf_descs, NULL)) < 0)
	{
		ep_in = NULL;
		ep_out = NULL;
		usb_ep_autoconfig_reset(cdev->gadget);
		return ret;
	}

	printk(KERN_INFO "GEP %s speed %s%s\n",
		(gadget_is_superspeed(c->cdev->gadget) ? "super" :
			(gadget_is_dualspeed(c->cdev->gadget) ? "high" : "full")),
		f->name, (gadget_is_otg(c->cdev->gadget) ? " (OTG)" : ""));
	return 0;
}
static void gep_free_func(struct usb_function *f)
{
	printk(KERN_INFO "GEP %s free function\n", f->name);
	usb_free_all_descriptors(f);
	ep_in = NULL;
	ep_out = NULL;
	usb_ep_autoconfig_reset(f->config->cdev->gadget);
}

static int gep_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	printk(KERN_INFO "GEP %s - cur intf: %u, set alt: %u\n", f->name, intf, alt);

	if ((ret = config_ep_by_speed(cdev->gadget, f, ep_out)) < 0)
	{
		return ret;
	}
	if ((ret = usb_ep_enable(ep_out)) < 0)
	{
		return ret;
	}
	printk(KERN_INFO "GEP ep 0x%x (%s) MaxPS=%d Ivl=%dus enabled\n",
		ep_out->address, ep_out->name, ep_out->maxpacket, COMPUTE_EP_IVL(ep_out));

	if ((ret = config_ep_by_speed(cdev->gadget, f, ep_in)) < 0)
	{
		usb_ep_disable(ep_out);
		return ret;
	}
	if ((ret = usb_ep_enable(ep_in)) < 0)
	{
		usb_ep_disable(ep_out);
		return ret;
	}
	printk(KERN_INFO "GEP ep 0x%x (%s) MaxPS=%d Ivl=%dus enabled\n",
		ep_in->address, ep_in->name, ep_in->maxpacket, COMPUTE_EP_IVL(ep_in));

	return 0;
}
static void gep_disable(struct usb_function *f)
{
	usb_ep_disable(ep_in);
	printk(KERN_INFO "GEP ep 0x%x (%s) disabled\n", ep_in->address, ep_in->name);
	usb_ep_disable(ep_out);
	printk(KERN_INFO "GEP ep 0x%x (%s) disabled\n", ep_out->address, ep_out->name);
	printk(KERN_INFO "GEP %s disabled\n", f->name);
}

static struct usb_function func =
{
	.name = "gep_intf",
	.bind = gep_bind,
	.free_func = gep_free_func, // Used as inverse of bind in absence of unbind
	.set_alt = gep_set_alt,
	.disable = gep_disable,
};

static int __init bbb_gep_bind(struct usb_composite_dev *cdev)
{
	int ret;

	/*
	 * Allocate string descriptor numbers.
	 * Note that string contents can be overridden by the composite_dev glue.
	 */
	if ((ret = usb_string_ids_tab(cdev, strings)) < 0)
		return ret;

	device_desc.iManufacturer = strings[USB_GADGET_MANUFACTURER_IDX].id;
	device_desc.iProduct = strings[USB_GADGET_PRODUCT_IDX].id;
	device_desc.iSerialNumber = strings[USB_GADGET_SERIAL_IDX].id;
	printk(KERN_INFO "GEP Mfg id: %d\n", device_desc.iManufacturer);
	printk(KERN_INFO "GEP Prod id: %d\n", device_desc.iProduct);
	printk(KERN_INFO "GEP S/N id: %d\n", device_desc.iSerialNumber);

	config.bmAttributes &= ~USB_CONFIG_ATT_WAKEUP;
	config.descriptors = NULL;

	usb_add_config_only(cdev, &config);

	return usb_add_function(&config, &func);
}

static int __exit bbb_gep_unbind(struct usb_composite_dev *cdev)
{
	usb_put_function(&func);
	return 0;
}

static __refdata struct usb_composite_driver bbb_gep_driver = {
	.name = "gadget_with_ep",
	.dev = &device_desc,
	.strings = strings_desc,
	.max_speed = USB_SPEED_HIGH,
	.bind = bbb_gep_bind,
	.unbind = __exit_p(bbb_gep_unbind),
};

static int __init bbb_gep_init(void)
{
	return usb_composite_probe(&bbb_gep_driver);
}
static void __exit bbb_gep_exit(void)
{
	usb_composite_unregister(&bbb_gep_driver);
}
module_init(bbb_gep_init);
module_exit(bbb_gep_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("USB Gadget Driver w/ EndPoints for BBB");
