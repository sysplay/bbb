#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define DRIVER_NAME "Sample_Pldrv"

#define RESOURCE1_START_ADDRESS 0x100000
#define RESOURCE1_END_ADDRESS 0x1FFFFF
#define RESOURCE2_START_ADDRESS 0x300000
#define RESOURCE2_END_ADDRESS 0x3FFFFF
#define SAMPLE_DEV_IRQNUM 6

int gpio_led = 53;

MODULE_LICENSE("GPL");
	
static struct platform_device sample_device = {
	.name = DRIVER_NAME,
	.id = -1,
	.dev = {
		.platform_data = &gpio_led,
	},	
};
int ourinitmodule(void)
{
	printk(KERN_ALERT "\n Welcome to sample Platform driver(device).... \n");
	platform_device_register(&sample_device);
	return 0;
}
void ourcleanupmodule(void)
{
	platform_device_unregister(&sample_device);
	printk(KERN_ALERT "\n Thanks....Exiting sample Platform(device) driver... \n");
}
module_init(ourinitmodule);
module_exit(ourcleanupmodule);
