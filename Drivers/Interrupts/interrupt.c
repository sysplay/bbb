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

static u8 gpio_sw = 72;
static int irq_line;

static irqreturn_t button_irq_handler(int irq, void *data)
{
	printk("GPIO value is %d\n", gpio_get_value(gpio_sw));
	return IRQ_HANDLED;
}


static int __init my_init(void)
{
	int irq_req_res;

	gpio_request_one(gpio_sw, GPIOF_IN, "button");
	if ( (irq_line = gpio_to_irq(gpio_sw)) < 0)
	{
		printk(KERN_ALERT "Gpio %d cannot be used as interrupt",gpio_sw);
		return -EINVAL;
	}

	if ( (irq_req_res = request_irq(irq_line, button_irq_handler, 
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "gpio_change_state", NULL)) < 0) 
	{
		printk(KERN_ERR "Keypad: registering irq failed\n");
		return -EINVAL;
	}
	
	return 0;
}

static void __exit my_cleanup(void)
{
	free_irq(irq_line, NULL);
	printk(KERN_INFO "Removing Interrupt driver\n");
}

module_init(my_init);
module_exit(my_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("GPIO Interrupts Demo");
