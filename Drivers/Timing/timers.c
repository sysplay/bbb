#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>

static struct timer_list my_timer;

void my_timer_callback(unsigned long data)
{
	printk("My Timer call-back called (%ld).\n", jiffies);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(200));
}

int init_module(void)
{
	int ret;

	printk("Timer module installing\n");
	setup_timer(&my_timer, my_timer_callback, 0);
	printk("Starting timer to fire in 200ms (%ld)\n", jiffies);
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(200));
	if (ret) 
		printk("Error in mod_timer\n");
	return 0;
}

void cleanup_module(void)
{
	int ret;
	
	ret = del_timer(&my_timer);
	if (ret)
		printk("The timer is still in use...\n");
	printk("Timer module uninstalling\n");
	return;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Kernel Timers Demo");
