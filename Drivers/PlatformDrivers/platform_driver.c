#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/platform_device.h>
#define DRIVER_NAME "Sample_Pldrv"

MODULE_LICENSE("GPL");

/**************/ 
static int sample_drv_probe(struct platform_device *pdev) {
	struct resource *res1, *res2;
	printk("Platform Driver Probe called\n");
	res1 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res1) {
		pr_err(" Specified Resource Not Available... 1\n");
		return -1;
	}
	printk(KERN_ALERT "\n Memory Area1\n");
	printk(KERN_ALERT "Start:%lx, End:%lx Size:%d\n", (unsigned long)res1->start, 
			(unsigned long)res1->end, resource_size(res1));
	res2 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if ((!res2)) {
		pr_err(" Specified Resource Not Available... 2\n");
		return -1;
	}
	printk(KERN_ALERT "\n Memory Area2\n");
	printk(KERN_ALERT "Start:%lx, End:%lx, size:%d\n", (unsigned long)res2->start, 
			(unsigned long)res2->end, resource_size(res2));
	printk(KERN_ALERT "\n IRQ Number of Device :%d\n", platform_get_irq(pdev, 0));
	return 0; 
}
static int sample_drv_remove(struct platform_device *pdev){
	return 0;
}

static struct platform_driver sample_pldriver = {
	.probe          = sample_drv_probe,
	.remove         = sample_drv_remove,
	.driver = {
		.name  = DRIVER_NAME,
	},
};
/**************/  

int ourinitmodule(void)
{
	printk(KERN_ALERT "\n Welcome to sample Platform driver.... \n");

	/* Registering with Kernel */
	platform_driver_register(&sample_pldriver);

	return 0;
}

void ourcleanupmodule(void)
{
	printk(KERN_ALERT "\n Thanks....Exiting sample Platform driver... \n");

	/* Unregistering from Kernel */
	platform_driver_unregister(&sample_pldriver);

	return;
}

module_init(ourinitmodule);
module_exit(ourcleanupmodule);
