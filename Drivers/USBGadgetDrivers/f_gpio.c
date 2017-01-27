#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/err.h>

#include "gpio.h"

#define BUF_LEN 64 // should be >= (sizeof(GPIOFData))

struct f_gpio
{
	struct usb_function function;

	struct usb_ep *in_ep;
	struct usb_ep *out_ep;
};

static inline struct f_gpio *func_to_gpio(struct usb_function *f)
{
	return container_of(f, struct f_gpio, function);
}

static struct usb_interface_descriptor intf_desc = {
	.bLength = sizeof(intf_desc),
	.bDescriptorType = USB_DT_INTERFACE,

	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
	/* .iInterface = DYNAMIC */
};

static struct usb_endpoint_descriptor ep_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
};
static struct usb_endpoint_descriptor ep_out_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *intf_descs[] = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &ep_out_desc,
	(struct usb_descriptor_header *) &ep_in_desc,
	NULL,
};

/* Function-specific strings */

static struct usb_string gpio_strings[] = {
	[0].s = "General Purpose Input / Output",
	{ } /* end of list */
};

static struct usb_gadget_strings gpio_strings_tab = {
	.language = 0x0409, /* en-us */
	.strings = gpio_strings,
};

static struct usb_gadget_strings *gpio_func_strings[] = {
	&gpio_strings_tab,
	NULL,
};

/*-------------------------------------------------------------------------*/

static void gpio_complete(struct usb_ep *ep, struct usb_request *req);

static struct usb_request *alloc_ep_req(struct usb_ep *ep, int len)
{
	struct usb_request *req;

	printk(KERN_INFO "GPIO alloc ep 0x%x req (%s)\n", ep->address, ep->name);
	req = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (req)
	{
		req->length = len;
		req->buf = kmalloc(req->length, GFP_ATOMIC);
		if (!req->buf)
		{
			usb_ep_free_request(ep, req);
			req = NULL;
		}
		else
		{
			req->complete = gpio_complete;
		}
	}
	return req;
}
static void free_ep_req(struct usb_ep *ep, struct usb_request *req)
{
	printk(KERN_INFO "GPIO free ep 0x%x req (%s)\n", ep->address, ep->name);
	kfree(req->buf);
	usb_ep_free_request(ep, req);
}

static int gpio_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	int id, ret;
	struct f_gpio *gpio = func_to_gpio(f);

	printk(KERN_INFO "GPIO %s bind\n", f->name);

	/* Allocate interface ID(s) */
	if ((id = usb_interface_id(c, f)) < 0)
		return id;
	intf_desc.bInterfaceNumber = id;
	printk(KERN_INFO "GPIO i/f id: %d\n", id);

	if ((id = usb_string_id(cdev)) < 0)
		return id;
	gpio_strings[0].id = id;
	intf_desc.iInterface = id;
	printk(KERN_INFO "GPIO Str id: %d\n", id);

	if (!(gpio->in_ep = usb_ep_autoconfig(cdev->gadget, &ep_in_desc)))
	{
		printk(KERN_ERR "GPIO %s: Can't autoconfigure ep_in on %s\n", f->name, cdev->gadget->name);
		return -ENODEV;
	}
	gpio->in_ep->driver_data = cdev; /* Claim */
	if (!(gpio->out_ep = usb_ep_autoconfig(cdev->gadget, &ep_out_desc)))
	{
		printk(KERN_ERR "GPIO %s: Can't autoconfigure ep_out on %s\n", f->name, cdev->gadget->name);
		gpio->in_ep = NULL;
		usb_ep_autoconfig_reset(cdev->gadget);
		return -ENODEV;
	}
	gpio->out_ep->driver_data = cdev; /* Claim */

	if ((ret = usb_assign_descriptors(f, NULL, intf_descs, NULL)) < 0)
	{
		gpio->out_ep = NULL;
		gpio->in_ep = NULL;
		usb_ep_autoconfig_reset(cdev->gadget);
		return ret;
	}

	printk(KERN_INFO "GPIO %s speed %s%s\n",
		(gadget_is_superspeed(c->cdev->gadget) ? "super" :
			(gadget_is_dualspeed(c->cdev->gadget) ? "high" : "full")),
		f->name, (gadget_is_otg(c->cdev->gadget) ? " (OTG)" : ""));
	return 0;
}
static void gpio_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_gpio *gpio = func_to_gpio(f);

	printk(KERN_INFO "GPIO %s unbind\n", f->name);
	usb_free_all_descriptors(f);
	gpio->out_ep = NULL;
	gpio->in_ep = NULL;
	usb_ep_autoconfig_reset(f->config->cdev->gadget);
}

static void gpio_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_gpio *gpio = ep->driver_data;
	GPIOFData *gfd = req->buf;
	int status;

	printk(KERN_INFO "GPIO %s (0x%02X) complete - status: %d, transfer: %d/%d\n",
		ep->name, ep->address, req->status, req->actual, req->length);
	switch (req->status)
	{
		case 0: /* Normal completion */
			if (ep == gpio->out_ep)
			{
				switch (gfd->cmd)
				{
					case _BBB_GPIO_GET:
					{
						struct usb_request *resp = alloc_ep_req(gpio->in_ep, sizeof(GPIOFData));
						if (resp)
						{
							gpio_request_one(gfd->d.num, GPIOF_IN, "bbb_gpio"); // Hack: To read the switch on GPIO2_8
							gfd->d.val = gpio_get_value(gfd->d.num);
							memcpy(resp->buf, gfd, sizeof(GPIOFData));
							if ((status = usb_ep_queue(gpio->in_ep, resp, GFP_ATOMIC)) == 0)
							{
								free_ep_req(ep, req);
								return;
							}
							else
							{
								/* "should never get here" */
								printk(KERN_ERR "failed to queue response into %s: %d\n",
									gpio->in_ep->name, status);
								free_ep_req(gpio->in_ep, resp);
							}
						}
						else
						{
							/* "should never get here" */
							printk(KERN_ERR "failed allocating memory for IN packets\n");
						}
						break;
					}
					case _BBB_GPIO_SET:
						gpio_set_value(gfd->d.num, gfd->d.val);
						break;
					default:
						/* "should never get here" */
						printk(KERN_ERR "invalid cmd 0x%x\n", gfd->cmd);
						break;
				}
				/* queue the buffer back for next OUT packet */
				if ((status = usb_ep_queue(gpio->out_ep, req, GFP_ATOMIC)) != 0)
				{
					/* "should never get here" */
					printk(KERN_ERR "failed to queue back into %s: %d\n",
						gpio->out_ep->name, status);
				}
				return;
			}
			else if (ep == gpio->in_ep)
			{
				struct usb_request *req_new;

				free_ep_req(ep, req);

				/* queue a buffer back for OUT packet */
				req_new = alloc_ep_req(gpio->out_ep, BUF_LEN);
				if (req_new)
				{
					if ((status = usb_ep_queue(gpio->out_ep, req_new, GFP_ATOMIC)) != 0)
					{
						/* "should never get here" */
						printk(KERN_ERR "failed to queue packet into %s: %d\n",
							gpio->out_ep->name, status);
						free_ep_req(gpio->out_ep, req_new);
					}
				}
				else
				{
					/* "should never get here" */
					printk(KERN_ERR "failed allocating memory for OUT packets\n");
				}
				return;
			}

			/* FALLTHROUGH on other (invalid) eps */

		default:
			printk(KERN_INFO "invalid %s gpio, %d/%d\n", ep->name,
				req->actual, req->length);
			/* FALLTHROUGH */

			/* NOTE: since this driver doesn't maintain an explicit record
			 * of requests it submitted (just maintains qlen count), we
			 * rely on the hardware driver to clean up on disconnect or
			 * endpoint disable.
			 */
		case -ECONNABORTED:		/* hardware forced ep reset */
		case -ECONNRESET:		/* request dequeued */
		case -ESHUTDOWN:		/* disconnect from host */
			free_ep_req(ep, req);
			return;
	}
}

static int enable_gpio(struct usb_composite_dev *cdev, struct f_gpio *gpio)
{
	int ret = 0;
	struct usb_ep *ep;
	struct usb_request *req;

	printk(KERN_INFO "GPIO entered enable\n");

	/* One endpoint writes responses back IN to the host */
	ep = gpio->in_ep;
	if ((ret = config_ep_by_speed(cdev->gadget, &(gpio->function), ep)) < 0)
		return ret;
	if ((ret = usb_ep_enable(ep)) < 0)
		return ret;
	ep->driver_data = gpio;

	/* One endpoint just reads OUT requests from the host */
	ep = gpio->out_ep;
	if ((ret = config_ep_by_speed(cdev->gadget, &(gpio->function), ep)) < 0)
		goto fail;

	if ((ret = usb_ep_enable(ep)) < 0)
	{
fail:
		ep = gpio->in_ep;
		usb_ep_disable(ep);
		ep->driver_data = NULL;
		return ret;
	}
	ep->driver_data = gpio;

	/* Allocate & queue one OUT buffer */
	if ((req = alloc_ep_req(ep, BUF_LEN)))
	{
		if ((ret = usb_ep_queue(ep, req, GFP_ATOMIC)) < 0)
			printk(KERN_ERR "%s queue req --> %d\n", ep->name, ret);
	}
	else
	{
		usb_ep_disable(ep);
		ep->driver_data = NULL;
		ret = -ENOMEM;
		goto fail;
	}

	printk(KERN_INFO "GPIO %s enabled\n", gpio->function.name);
	return ret;
}
static int gpio_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_gpio *gpio = func_to_gpio(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	printk(KERN_INFO "GPIO set alt\n");
	if (gpio->in_ep->driver_data != gpio)
	/* we know there is just the default alt, so enable it */
	{
		return enable_gpio(cdev, gpio);
	}
	else
		return 0;
}

static void disable_ep(struct usb_ep *ep)
{
	if (ep->driver_data)
	{
		usb_ep_disable(ep);
		printk(KERN_INFO "GPIO disable ep 0x%x req (%s)\n", ep->address, ep->name);
		ep->driver_data = NULL;
	}
}
static void gpio_disable(struct usb_function *f)
{
	struct f_gpio *gpio = func_to_gpio(f);

	printk(KERN_INFO "GPIO entered disable\n");
	disable_ep(gpio->in_ep);
	disable_ep(gpio->out_ep);
	printk(KERN_INFO "GPIO %s disabled\n", gpio->function.name);
}

static void gpio_free_func(struct usb_function *f)
{
	struct f_gpio *gpio = func_to_gpio(f);

	printk(KERN_INFO "GPIO free\n");
	kfree(gpio);
}
static struct usb_function *gpio_alloc_func(struct usb_function_instance *fi)
{
	struct f_gpio *gpio;

	printk(KERN_INFO "GPIO alloc\n");
	gpio = kzalloc(sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return ERR_PTR(-ENOMEM);

	gpio->function.name = "gpio";
	gpio->function.bind = gpio_bind;
	gpio->function.unbind = gpio_unbind;
	gpio->function.set_alt = gpio_set_alt;
	gpio->function.disable = gpio_disable;
	gpio->function.strings = gpio_func_strings;

	gpio->function.free_func = gpio_free_func;

	return &gpio->function;
}

static void gpio_free_instance(struct usb_function_instance *fi)
{
	printk(KERN_INFO "GPIO free instance\n");
	kfree(fi);
}
static struct usb_function_instance *gpio_alloc_instance(void)
{
	struct usb_function_instance *fi;

	printk(KERN_INFO "GPIO alloc instance\n");
	fi = kzalloc(sizeof(*fi), GFP_KERNEL);
	if (!fi)
		return ERR_PTR(-ENOMEM);
	fi->free_func_inst = gpio_free_instance;
	return fi;
}

DECLARE_USB_FUNCTION(gpio, gpio_alloc_instance, gpio_alloc_func);

int __init gpio_init(void)
{
	printk(KERN_INFO "GPIO init\n");
	return usb_function_register(&gpiousb_func);
}
void __exit gpio_exit(void)
{
	usb_function_unregister(&gpiousb_func);
	printk(KERN_INFO "GPIO exit\n");
}
module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("USB Gadget Driver Function for BBB GPIO Device");
