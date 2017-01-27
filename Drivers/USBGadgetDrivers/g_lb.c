#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/err.h>

/* OTG test device IDs */
#define BBB_LB_VID	0x1A0A
#define BBB_LB_PID	0xBADD

static struct usb_device_descriptor device_desc = {
	.bLength = sizeof(device_desc),
	.bDescriptorType = USB_DT_DEVICE,

	.bcdUSB = cpu_to_le16(0x0200),
	.bDeviceClass = USB_CLASS_VENDOR_SPEC,

	.idVendor = cpu_to_le16(BBB_LB_VID),
	.idProduct = cpu_to_le16(BBB_LB_PID),
	.bNumConfigurations = 1,
};

static struct usb_string strings[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "SysPlay eLearning Academy for You <workshop@sysplay.in>",
	[USB_GADGET_PRODUCT_IDX].s = "BBB LB Device",
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
	.label = "lb_conf",
	.bConfigurationValue = 1,
	.bmAttributes = USB_CONFIG_ATT_SELFPOWER,
};

static struct usb_function_instance *func_inst_lb;
static struct usb_function *func_lb;

static int __init bbb_lb_bind(struct usb_composite_dev *cdev)
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
	printk(KERN_INFO "LB Mfg id: %d\n", device_desc.iManufacturer);
	printk(KERN_INFO "LB Prod id: %d\n", device_desc.iProduct);
	printk(KERN_INFO "LB S/N id: %d\n", device_desc.iSerialNumber);

	config.bmAttributes &= ~USB_CONFIG_ATT_WAKEUP;
	config.descriptors = NULL;

	usb_add_config_only(cdev, &config);

	if (IS_ERR(func_inst_lb = usb_get_function_instance("lb")))
	{
		return PTR_ERR(func_inst_lb);
	}

	if (IS_ERR(func_lb = usb_get_function(func_inst_lb)))
	{
		usb_put_function_instance(func_inst_lb);
		return PTR_ERR(func_lb);
	}

	if ((ret = usb_add_function(&config, func_lb)) < 0)
	{
		usb_put_function(func_lb);
		usb_put_function_instance(func_inst_lb);
		return ret;
	}

	return 0;
}

static int bbb_lb_unbind(struct usb_composite_dev *cdev)
{
	usb_put_function(func_lb);
	usb_put_function_instance(func_inst_lb);
	return 0;
}

static __refdata struct usb_composite_driver bbb_lb_driver = {
	.name = "loopback",
	.dev = &device_desc,
	.strings = strings_desc,
	.max_speed = USB_SPEED_HIGH,
	.bind = bbb_lb_bind,
	.unbind = __exit_p(bbb_lb_unbind),
};

static int __init bbb_lb_init(void)
{
	return usb_composite_probe(&bbb_lb_driver);
}
static void __exit bbb_lb_exit(void)
{
	usb_composite_unregister(&bbb_lb_driver);
}
module_init(bbb_lb_init);
module_exit(bbb_lb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("USB Gadget Driver for BBB LoopBack Device");
