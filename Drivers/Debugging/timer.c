#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>

static struct timer_list my_timer;
static int count = 0;

void my_timer_callback(unsigned long data)
{
	count++;
	printk(KERN_INFO "Timer call-back called (%lu) - %d times.\n", jiffies, count);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
}

static int __init debug_init(void)
{
	int ret;

	printk(KERN_INFO "Timer module installing\n");
	setup_timer(&my_timer, my_timer_callback, 0);
	printk(KERN_INFO "Starting timer to fire in 2s (%lu)\n", jiffies);
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
	if (ret) 
		printk(KERN_ERR "Error in mod_timer\n");
	return 0;
}

static void __exit debug_exit(void)
{
	int ret;
	
	ret = del_timer(&my_timer);
	if (ret)
		printk(KERN_INFO "The timer is still in use...\n");
	printk(KERN_INFO "Timer module uninstalling\n");
}

module_init(debug_init);
module_exit(debug_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Timer Debug Driver");
