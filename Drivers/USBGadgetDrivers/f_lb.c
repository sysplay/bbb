#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/errno.h>

#define EP_MAXPS 64

#define EP_BULK

#ifndef EP_BULK
#define EP_XFER USB_ENDPOINT_XFER_INT
//#define EP_XFER USB_ENDPOINT_XFER_ISOC
#define EP_IVL 1
#define COMPUTE_EP_IVL(ep) (125 * (1 << ((ep)->desc->bInterval - 1)))
#else
#define EP_XFER USB_ENDPOINT_XFER_BULK
#define EP_IVL 0
#define COMPUTE_EP_IVL(ep) (125 * ((ep)->desc->bInterval))
#endif

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
static struct usb_request *ep_in_req, *ep_out_req;
static uint8_t buf[EP_MAXPS];

static int lb_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	int id, ret;

	printk(KERN_INFO "LB %s bind\n", f->name);

	/* Allocate interface ID(s) */
	if ((id = usb_interface_id(c, f)) < 0)
		return id;
	intf_desc.bInterfaceNumber = id;
	printk(KERN_INFO "LB i/f id: %d\n", id);

	if (!(ep_out = usb_ep_autoconfig(cdev->gadget, &ep_out_desc)))
	{
		printk(KERN_ERR "LB %s: Can't autoconfigure ep_out on %s\n", f->name, cdev->gadget->name);
		return -ENODEV;
	}
	ep_out->driver_data = cdev; /* Claim */
	if (!(ep_in = usb_ep_autoconfig(cdev->gadget, &ep_in_desc)))
	{
		printk(KERN_ERR "LB %s: Can't autoconfigure ep_in on %s\n", f->name, cdev->gadget->name);
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

	printk(KERN_INFO "LB %s speed %s%s\n",
		(gadget_is_superspeed(c->cdev->gadget) ? "super" :
			(gadget_is_dualspeed(c->cdev->gadget) ? "high" : "full")),
		f->name, (gadget_is_otg(c->cdev->gadget) ? " (OTG)" : ""));
	return 0;
}
static void lb_unbind(struct usb_configuration *c, struct usb_function *f)
{
	printk(KERN_INFO "LB %s unbind\n", f->name);
	usb_free_all_descriptors(f);
	ep_in = NULL;
	ep_out = NULL;
	usb_ep_autoconfig_reset(f->config->cdev->gadget);
}

static void lb_out_complete(struct usb_ep *ep, struct usb_request *req)
{
	int i;

	printk(KERN_INFO "LB %s (0x%02X) complete - status: %d, transfer: %d/%d\n",
		ep->name, ep->address, req->status, req->actual, req->length);
	switch (req->status)
	{
		case 0: /* Normal completion */
			printk(KERN_INFO "Data Out: ");
			for (i = 0; i < req->actual; i++)
			{
				printk("%c", ((char *)(req->buf))[i]);
			}
			printk("\n");
			ep_in_req->length = req->actual;
			usb_ep_queue(ep_in, ep_in_req, GFP_ATOMIC);
			printk(KERN_INFO "LB ep 0x%x (%s) request queued\n", ep_in->address, ep_in->name);
			break;
		default:
			printk(KERN_INFO "LB Ref - hw forced ep reset: %d, req dequeued: %d, disconnect from host: %d\n",
				-ECONNABORTED, -ECONNRESET, -ESHUTDOWN);
			break;
	}
}
static void lb_in_complete(struct usb_ep *ep, struct usb_request *req)
{
	int i;

	printk(KERN_INFO "LB %s (0x%02X) complete - status: %d, transfer: %d/%d\n",
		ep->name, ep->address, req->status, req->actual, req->length);
	switch (req->status)
	{
		case 0:
			printk(KERN_INFO "Data In: ");
			for (i = 0; i < req->actual; i++)
			{
				printk("%c", ((char *)(req->buf))[i]);
			}
			printk("\n");
			usb_ep_queue(ep_out, ep_out_req, GFP_ATOMIC);
			printk(KERN_INFO "LB ep 0x%x (%s) request queued\n", ep_out->address, ep_out->name);
			break;
		default:
			printk(KERN_INFO "LB Ref - hw forced ep reset: %d, req dequeued: %d, disconnect from host: %d\n",
				-ECONNABORTED, -ECONNRESET, -ESHUTDOWN);
			break;
	}
}

static int setup_ep_out_req(struct usb_ep *ep)
{
	int ret;

	if (!(ep_out_req = usb_ep_alloc_request(ep, GFP_ATOMIC)))
	{
		return -ENOMEM;
	}
	ep_out_req->length = ep->maxpacket;
	ep_out_req->buf = buf;
	ep_out_req->complete = lb_out_complete;
	if ((ret = usb_ep_queue(ep, ep_out_req, GFP_ATOMIC)) < 0)
	{
		printk(KERN_ERR "LB %s queue req err %d\n", ep->name, ret);
		usb_ep_free_request(ep, ep_out_req);
		ep_out_req = NULL;
		return ret;
	}
	printk(KERN_INFO "LB ep 0x%x (%s) request allocated & queued\n", ep->address, ep->name);
	return 0;
}
static void cleanup_ep_out_req(struct usb_ep *ep)
{
	usb_ep_dequeue(ep, ep_out_req);
	usb_ep_free_request(ep, ep_out_req);
	ep_out_req = NULL;
	printk(KERN_INFO "LB ep 0x%x (%s) request (dequeued &) freed\n", ep->address, ep->name);
}
static int setup_ep_in_req(struct usb_ep *ep)
{
	if (!(ep_in_req = usb_ep_alloc_request(ep, GFP_ATOMIC)))
	{
		return -ENOMEM;
	}
	ep_in_req->length = ep->maxpacket;
	ep_in_req->buf = buf;
	ep_in_req->complete = lb_in_complete;
	printk(KERN_INFO "LB ep 0x%x (%s) request allocated\n", ep->address, ep->name);
	return 0;
}
static void cleanup_ep_in_req(struct usb_ep *ep)
{
	usb_ep_dequeue(ep, ep_in_req);
	usb_ep_free_request(ep, ep_in_req);
	ep_in_req = NULL;
	printk(KERN_INFO "LB ep 0x%x (%s) request (dequeued &) freed\n", ep->address, ep->name);
}

static int lb_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	printk(KERN_INFO "LB %s - cur intf: %u, set alt: %u\n", f->name, intf, alt);

	if ((ret = config_ep_by_speed(cdev->gadget, f, ep_out)) < 0)
		return ret;
	if ((ret = usb_ep_enable(ep_out)) < 0)
		return ret;
	printk(KERN_INFO "LB ep 0x%x (%s) MaxPS=%d Ivl=%dus enabled\n",
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
	printk(KERN_INFO "LB ep 0x%x (%s) MaxPS=%d Ivl=%dus enabled\n",
		ep_in->address, ep_in->name, ep_in->maxpacket, COMPUTE_EP_IVL(ep_in));

	if ((ret = setup_ep_out_req(ep_out)) < 0)
	{
		usb_ep_disable(ep_in);
		usb_ep_disable(ep_out);
		return ret;
	}
	if ((ret = setup_ep_in_req(ep_in)) < 0)
	{
		cleanup_ep_out_req(ep_out);
		usb_ep_disable(ep_in);
		usb_ep_disable(ep_out);
		return ret;
	}

	return 0;
}
static void lb_disable(struct usb_function *f)
{
	cleanup_ep_in_req(ep_in);
	cleanup_ep_out_req(ep_out);
	usb_ep_disable(ep_in);
	printk(KERN_INFO "LB ep 0x%x (%s) disabled\n", ep_in->address, ep_in->name);
	usb_ep_disable(ep_out);
	printk(KERN_INFO "LB ep 0x%x (%s) disabled\n", ep_out->address, ep_out->name);
	printk(KERN_INFO "LB %s disabled\n", f->name);
}

static void lb_free_func(struct usb_function *f)
{
	printk(KERN_INFO "LB %s free function\n", f->name);
}
static struct usb_function lb_func =
{
	.name = "lb_intf",
	.bind = lb_bind,
	.unbind = lb_unbind,
	.set_alt = lb_set_alt,
	.disable = lb_disable,

	.free_func = lb_free_func,
};
static struct usb_function *lb_alloc_func(struct usb_function_instance *fi)
{
	printk(KERN_INFO "LB alloc function\n");
	return &lb_func;
}

static void lb_free_func_inst(struct usb_function_instance *fi)
{
	printk(KERN_INFO "LB free function instance\n");
}
static struct usb_function_instance lb_func_inst =
{
	.free_func_inst = lb_free_func_inst,
};
static struct usb_function_instance *lb_alloc_func_inst(void)
{
	printk(KERN_INFO "LB alloc function instance\n");
	return &lb_func_inst;
}

DECLARE_USB_FUNCTION(lb, lb_alloc_func_inst, lb_alloc_func);

static int __init lb_init(void)
{
	printk(KERN_INFO "LB init\n");
	return usb_function_register(&lbusb_func);
}
static void __exit lb_exit(void)
{
	usb_function_unregister(&lbusb_func);
	printk(KERN_INFO "LB exit\n");
}
module_init(lb_init);
module_exit(lb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("USB Gadget Function Driver for BBB LoopBack Device");
