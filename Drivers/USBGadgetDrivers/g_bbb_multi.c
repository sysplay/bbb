#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/err.h>

static char *function = "gpio";

module_param_named(function, function, charp, 0);

USB_GADGET_COMPOSITE_OPTIONS();

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

static struct usb_function_instance *func_inst_generic;
static struct usb_function *func_generic;
static struct usb_function_instance *func_inst_lb;
static struct usb_function *func_lb;

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

	config.bmAttributes &= ~USB_CONFIG_ATT_WAKEUP;
	config.descriptors = NULL;

	usb_add_config_only(cdev, &config);

	if (IS_ERR(func_inst_generic = usb_get_function_instance(function)))
	{
		return PTR_ERR(func_inst_generic);
	}

	if (IS_ERR(func_generic = usb_get_function(func_inst_generic)))
	{
		ret = PTR_ERR(func_generic);
		goto err_put_func_inst_generic;
	}

	if (IS_ERR(func_inst_lb = usb_get_function_instance("lb")))
	{
		ret = PTR_ERR(func_inst_lb);
		goto err_put_func_generic;
	}

	if (IS_ERR(func_lb = usb_get_function(func_inst_lb)))
	{
		ret = PTR_ERR(func_lb);
		goto err_put_func_inst_lb;
	}

	if ((ret = usb_add_function(&config, func_generic)) < 0)
		goto err_put_func_lb;

	if ((ret = usb_add_function(&config, func_lb)) < 0)
		goto err_put_func_lb;

	usb_composite_overwrite_options(cdev, &coverwrite);

	return 0;

err_put_func_lb:
	usb_put_function(func_lb);
	func_lb = NULL;
err_put_func_inst_lb:
	usb_put_function_instance(func_inst_lb);
	func_inst_lb = NULL;
err_put_func_generic:
	usb_put_function(func_generic);
	func_generic = NULL;
err_put_func_inst_generic:
	usb_put_function_instance(func_inst_generic);
	func_inst_generic = NULL;
	return ret;
}

static int __exit bbb_gm_unbind(struct usb_composite_dev *cdev)
{
	usb_put_function(func_lb);
	usb_put_function_instance(func_inst_lb);
	usb_put_function(func_generic);
	usb_put_function_instance(func_inst_generic);
	return 0;
}

static __refdata struct usb_composite_driver bbb_gm_driver = {
	.name = "gadget_multi",
	.dev = &device_desc,
	.strings = strings_desc,
	.max_speed = USB_SPEED_HIGH,
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
