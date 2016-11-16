#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/slab.h>

//for platform drivers....
#include <linux/platform_device.h>
#define DRIVER_NAME "Sample_Pldrv"
static struct device_attribute led_attr;


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

static ssize_t gpio_led_clear(struct device *child, 
		struct device_attribute *attr, const char *buf, size_t count)
{       
	struct my_gpio_leds *led_data = dev_get_drvdata(child);
	printk("Led Clear called\n");
	gpio_set_value(led_data->leds[0].gpio, 0);
	return 0;
}

//static void gpio_led_set(struct led_classdev *led_cdev,
//		enum led_brightness value)
static ssize_t gpio_led_set(struct device *child, 
		struct device_attribute *attr, char *buf)
{       
	struct my_gpio_leds *led_data = dev_get_drvdata(child);
	printk("Led Set called on %d\n",led_data->leds[0].gpio);
	gpio_set_value(led_data->leds[0].gpio, 1);
	return 0;
}


static int create_gpio_led(const struct gpio_led *template,
		struct gpio_led_data *led_dat, struct device *parent)
{
	int ret, state;

	ret = gpio_direction_output(led_dat->gpio, 1);
	if (ret < 0) {
		printk("GPIO direction setting failed\n");
		goto err;
	}
	led_dat->gpio = template->gpio;
	led_attr.attr.name = template->name;
	led_attr.attr.mode = 0644;
	led_attr.show = gpio_led_set;
	led_attr.store = gpio_led_clear;
	if (device_create_file(parent, &led_attr) != 0 )
	{
		printk(KERN_ALERT "Sysfs Attribute Led- %s\n", template->name);
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
		//priv->leds[0].gpio = pdata->leds[0].gpio; 
#if 1
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
#endif
	} else {
		return -ENODEV;
	}

	platform_set_drvdata(pdev, priv);

	return 0;
}

static int __devexit gpio_led_remove(struct platform_device *pdev)
{
#if 1
	struct my_gpio_leds *priv = dev_get_drvdata(&pdev->dev);
	int i;

	for (i = 0; i < priv->num_leds; i++)
		delete_gpio_led(&priv->leds[i]);

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(priv);
#endif
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
