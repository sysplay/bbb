#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/composite.h>

/* OTG test device IDs */
#define BBB_GM_VID	0x1A0A
#define BBB_GM_PID	0xBADD

static struct usb_device_descriptor device_desc = {
	.bLength = sizeof(device_desc),
	.bDescriptorType = USB_DT_DEVICE,

	.bcdUSB = cpu_to_le16(0x0200),
	.bDeviceClass = USB_CLASS_VENDOR_SPEC,

	.idVendor = cpu_to_le16(BBB_GM_VID),
	.idProduct = cpu_to_le16(BBB_GM_PID),
	.bNumConfigurations = 1,
};

static struct usb_string strings[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "SysPlay eLearning Academy for You <workshop@sysplay.in>",
	[USB_GADGET_PRODUCT_IDX].s = "BBB GM Device",
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
	.label = "gm_conf",
	.bConfigurationValue = 1,
	.bmAttributes = USB_CONFIG_ATT_SELFPOWER,
};

static struct usb_interface_descriptor intf_desc = {
	.bLength = sizeof(intf_desc),
	.bDescriptorType = USB_DT_INTERFACE,

	.bNumEndpoints = 0,
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
};

static struct usb_interface_descriptor intf_desc2 = {
	.bLength = sizeof(intf_desc2),
	.bDescriptorType = USB_DT_INTERFACE,

	.bNumEndpoints = 0,
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
};

static struct usb_descriptor_header *intf_descs[] = {
	(struct usb_descriptor_header *) &intf_desc,
	NULL,
};
static struct usb_descriptor_header *intf_descs2[] = {
	(struct usb_descriptor_header *) &intf_desc2,
	NULL,
};

/*-------------------------------------------------------------------------*/

static int gm_bind(struct usb_configuration *c, struct usb_function *f)
{
	int id, ret;

	printk(KERN_INFO "GM %s bind\n", f->name);

	/* Allocate interface ID(s) */
	if ((id = usb_interface_id(c, f)) < 0)
		return id;
	intf_desc.bInterfaceNumber = id;
	printk(KERN_INFO "GM i/f id: %d\n", id);

	//ret = usb_assign_descriptors(f, intf_descs, NULL, NULL); // FS >= USB 1.0
	ret = usb_assign_descriptors(f, NULL, intf_descs, NULL); // HS >= USB 2.0
	//ret = usb_assign_descriptors(f, NULL, NULL, intf_descs); // SS >= USB 3.0
	if (ret < 0)
		return ret;

	printk(KERN_INFO "GM %s speed %s%s\n",
		(gadget_is_superspeed(c->cdev->gadget) ? "super" :
			(gadget_is_dualspeed(c->cdev->gadget) ? "high" : "full")),
		f->name, (gadget_is_otg(c->cdev->gadget) ? " (OTG)" : ""));
	return 0;
}
static int gm_bind2(struct usb_configuration *c, struct usb_function *f)
{
	int id, ret;

	printk(KERN_INFO "GM %s bind\n", f->name);

	/* Allocate interface ID(s) */
	if ((id = usb_interface_id(c, f)) < 0)
		return id;
	intf_desc2.bInterfaceNumber = id;
	printk(KERN_INFO "GM i/f id: %d\n", id);

	//ret = usb_assign_descriptors(f, intf_descs, NULL, NULL); // FS >= USB 1.0
	ret = usb_assign_descriptors(f, NULL, intf_descs2, NULL); // HS >= USB 2.0
	//ret = usb_assign_descriptors(f, NULL, NULL, intf_descs); // SS >= USB 3.0
	if (ret < 0)
		return ret;

	printk(KERN_INFO "GM %s speed %s%s\n",
		(gadget_is_superspeed(c->cdev->gadget) ? "super" :
			(gadget_is_dualspeed(c->cdev->gadget) ? "high" : "full")),
		f->name, (gadget_is_otg(c->cdev->gadget) ? " (OTG)" : ""));
	return 0;
}
static void gm_free_func(struct usb_function *f)
{
	printk(KERN_INFO "GM %s free function\n", f->name);
	usb_free_all_descriptors(f);
}

static int gm_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	printk(KERN_INFO "GM %s - cur intf: %u, set alt: %u\n", f->name, intf, alt);
	return 0;
}
static void gm_disable(struct usb_function *f)
{
	printk(KERN_INFO "GM %s disabled\n", f->name);
}

static struct usb_function func =
{
	.name = "gm_intf",
	.bind = gm_bind,
	.free_func = gm_free_func, // Used as inverse of bind in absence of unbind
	.set_alt = gm_set_alt,
	.disable = gm_disable,
};
static struct usb_function func2 =
{
	.name = "gm_intf2",
	.bind = gm_bind2,
	.free_func = gm_free_func, // Used as inverse of bind in absence of unbind
	.set_alt = gm_set_alt,
	.disable = gm_disable,
};

static int __init bbb_gm_bind(struct usb_composite_dev *cdev)
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
	printk(KERN_INFO "GM Mfg id: %d\n", device_desc.iManufacturer);
	printk(KERN_INFO "GM Prod id: %d\n", device_desc.iProduct);
	printk(KERN_INFO "GM S/N id: %d\n", device_desc.iSerialNumber);

	config.bmAttributes &= ~USB_CONFIG_ATT_WAKEUP;
	config.descriptors = NULL;

	usb_add_config_only(cdev, &config);

	if ((ret = usb_add_function(&config, &func)) < 0)
	{
		return ret;
	}
	if ((ret = usb_add_function(&config, &func2)) < 0)
	{
		usb_put_function(&func);
		return ret;
	}

	return 0;
}

static int __exit bbb_gm_unbind(struct usb_composite_dev *cdev)
{
	usb_put_function(&func2);
	usb_put_function(&func);
	return 0;
}

static __refdata struct usb_composite_driver bbb_gm_driver = {
	.name = "gadget_multi",
	.dev = &device_desc,
	.strings = strings_desc,
	//.max_speed = USB_SPEED_FULL,
	.max_speed = USB_SPEED_HIGH,
	//.max_speed = USB_SPEED_SUPER,
	.bind = bbb_gm_bind,
	.unbind = __exit_p(bbb_gm_unbind),
};

static int __init bbb_gm_init(void)
{
	return usb_composite_probe(&bbb_gm_driver);
}
static void __exit bbb_gm_exit(void)
{
	usb_composite_unregister(&bbb_gm_driver);
}
module_init(bbb_gm_init);
module_exit(bbb_gm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("USB Gadget Driver w/ Multiple Interfaces for BBB");
