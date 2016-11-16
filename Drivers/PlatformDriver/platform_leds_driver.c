#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/slab.h>

//for platform drivers....
#include <linux/platform_device.h>
#define DRIVER_NAME "Sample_Pldrv"

MODULE_LICENSE("GPL");
struct gpio_led_data {
	struct led_classdev cdev;
	unsigned gpio;
};

struct my_gpio_leds {
	int num_leds;           
	struct gpio_led_data leds[];
}; 

static inline int sizeof_gpio_leds_priv(int num_leds)
{
	return sizeof(struct my_gpio_leds) +
	(sizeof(struct gpio_led_data) * num_leds);
}

static void gpio_led_set(struct led_classdev *led_cdev,
		enum led_brightness value)
{       
	struct gpio_led_data *led_dat =
		container_of(led_cdev, struct gpio_led_data, cdev);

	if (value == LED_OFF)
		gpio_set_value(led_dat->gpio, 0);
	else    
		gpio_set_value(led_dat->gpio, 1);
}


static int create_gpio_led(const struct gpio_led *template,
		struct gpio_led_data *led_dat, struct device *parent)
{
	int ret, state;

	led_dat->gpio = -1;

	/* skip leds that aren't available */
	if (!gpio_is_valid(template->gpio)) {
		printk(KERN_INFO "Skipping unavailable LED gpio %d (%s)\n",
				template->gpio, template->name);
		return 0;
	}
	led_dat->cdev.name = template->name;
	led_dat->cdev.default_trigger = template->default_trigger;
	led_dat->gpio = template->gpio;
	led_dat->cdev.brightness_set = gpio_led_set;
	led_dat->cdev.brightness = LED_FULL;

	ret = gpio_direction_output(led_dat->gpio, 1);
	if (ret < 0) {
		printk("GPIO direction setting failed\n");
		goto err;
	}


	ret = led_classdev_register(parent, &led_dat->cdev);
	if (ret < 0) {
		printk("Class creation failed\n");
		goto err;
	}

	return 0;
err:
	gpio_free(led_dat->gpio);
	return ret;
}

static void delete_gpio_led(struct gpio_led_data *led)
{
	if (!gpio_is_valid(led->gpio))
		return;
	led_classdev_unregister(&led->cdev);
	gpio_free(led->gpio);
}


static int __devinit gpio_led_probe(struct platform_device *pdev)
{
	struct gpio_led_platform_data *pdata = pdev->dev.platform_data;
	struct my_gpio_leds *priv;
	int i, ret = 0;

	if (pdata && pdata->num_leds) {
		priv = kzalloc(sizeof_gpio_leds_priv(pdata->num_leds),
				GFP_KERNEL);
		if (!priv)
			return -ENOMEM;

		priv->num_leds = pdata->num_leds;
		for (i = 0; i < priv->num_leds; i++) {
			ret = create_gpio_led(&pdata->leds[i],
					&priv->leds[i],
					&pdev->dev);
			if (ret < 0) {
				/* On failure: unwind the led creations */
				for (i = i - 1; i >= 0; i--)
					delete_gpio_led(&priv->leds[i]);
				kfree(priv);
				return ret;
			}
		}
	} else {
		return -ENODEV;
	}

	platform_set_drvdata(pdev, priv);

	return 0;
}

static int __devexit gpio_led_remove(struct platform_device *pdev)
{
	struct my_gpio_leds *priv = dev_get_drvdata(&pdev->dev);
	int i;

	for (i = 0; i < priv->num_leds; i++)
		delete_gpio_led(&priv->leds[i]);

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(priv);

	return 0;
}


/**************/  
static struct platform_driver gpio_led_driver = {
	.probe          = gpio_led_probe,
	.remove         = __devexit_p(gpio_led_remove),
	.driver         = {
		.name   = "my-leds",
		.owner  = THIS_MODULE,
	},
};

MODULE_ALIAS("platform:my-leds");


int ourinitmodule(void)
{
	printk(KERN_ALERT "\n Welcome to sample Platform driver.... \n");
	/* Registering with Kernel */
	return platform_driver_register(&gpio_led_driver);
}

void ourcleanupmodule(void)
{
	printk(KERN_ALERT "\n Thanks....Exiting sample Platform driver... \n");
	/* Unregistering from Kernel */
	platform_driver_unregister(&gpio_led_driver);
}

module_init(ourinitmodule);
module_exit(ourcleanupmodule);
