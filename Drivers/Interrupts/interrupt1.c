#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>		/* printk() */
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/moduleparam.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <linux/semaphore.h>
#include <linux/sched.h>

#include <linux/gpio.h>

static u8 gpio_sw = 7;
static u8 gpio_sw1 = 72;
static int irq_line, irq_line1;

static irqreturn_t button_irq_handler(int irq, void *data)
{
	if ((int) (data) == 1)
		printk("GPIO value on pin 7 is %d\n", gpio_get_value(gpio_sw));
	else
		printk("GPIO value on pin 72 is %d\n", gpio_get_value(gpio_sw1));
	return IRQ_HANDLED;
}


static int __init my_init(void)
{
	int irq_req_res;

	gpio_request_one(gpio_sw, GPIOF_IN, "button");
	gpio_request_one(gpio_sw1, GPIOF_IN, "button1");
	if ( (irq_line = gpio_to_irq(gpio_sw)) < 0){
		printk(KERN_ALERT "Gpio %d cannot be used as interrupt",gpio_sw);
		return -EINVAL;
	}

	if ( (irq_req_res = request_irq(irq_line, button_irq_handler, 
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "gpio_change_state", (void *)1)) < 0) 
	{
		printk(KERN_ERR "Keypad: registering irq failed\n");
		return -EINVAL;
	}
	if ( (irq_line1 = gpio_to_irq(gpio_sw1)) < 0)
	{
		printk(KERN_ALERT "Gpio %d cannot be used as interrupt",gpio_sw1);
		return -EINVAL;
	}

	if ( (irq_req_res = request_irq(irq_line1, button_irq_handler, 
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "gpioi1_change_state", (void *)2)) < 0) 
	{
		printk(KERN_ERR "Keypad: registering irq 2 failed\n");
		return -EINVAL;
	}
	
	return 0;
}

static void __exit my_cleanup(void)
{
	free_irq(irq_line, NULL);
	free_irq(irq_line1, NULL);
	printk(KERN_INFO "Removing Keypad driver\n");
}

module_init(my_init);
module_exit(my_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Muliple IRQs Demo");
