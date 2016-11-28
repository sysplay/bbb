#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/platform_device.h>
#include <linux/leds.h>

MODULE_LICENSE("GPL");

/* Specifying my resources information */
static struct gpio_led gpio_leds[] = {
	{
		.name                   = "myled",
		.default_trigger        = "mmc0",
		.gpio                   = 150,
	},
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds           = gpio_leds,
	.num_leds       = ARRAY_SIZE(gpio_leds),
};

static struct platform_device leds_gpio = {
	.name   = "my-leds",
	.id     = -1,
	.dev    = {
		.platform_data  = &gpio_led_info,
	},
};


int ourinitmodule(void)
{
	printk(KERN_ALERT "\n Welcome to sample Platform driver(device).... \n");
	platform_device_register(&leds_gpio);
	return 0;
}
void ourcleanupmodule(void)
{
	platform_device_unregister(&leds_gpio);
	printk(KERN_ALERT "\n Thanks....Exiting sample Platform(device) driver... \n");
}
module_init(ourinitmodule);
module_exit(ourcleanupmodule);
