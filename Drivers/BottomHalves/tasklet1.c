#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/interrupt.h>

static struct timer_list my_timer;
static struct tasklet_struct my_tasklet1;
static struct tasklet_struct my_tasklet2;

void my_timer_callback(unsigned long data)
{
	printk("Scheduling tasklets\n");
	tasklet_schedule(&my_tasklet1);
	tasklet_hi_schedule(&my_tasklet2);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(200));
}

void tsklt_fn(unsigned long data)
{
	printk("Tasklet %lu scheduled\n", data);
	return;
}


int init_module(void)
{
	int ret;

	printk("Timer module installing\n");
	setup_timer(&my_timer, my_timer_callback, 0);
	tasklet_init(&my_tasklet1, tsklt_fn, 1);
	tasklet_init(&my_tasklet2, tsklt_fn, 2);
	printk("Starting timer to fire in 200ms (%ld)\n", jiffies);
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(200));
	if (ret) 
		printk("Error in mod_timer\n");
	return 0;
}

void cleanup_module(void)
{
	int ret;
	
	ret = del_timer_sync(&my_timer);
	if (ret)
		printk("The timer is still in use...\n");
	tasklet_kill(&my_tasklet1);
	tasklet_kill(&my_tasklet2);
	printk("Timer module uninstalling\n");
	return;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Tasklet Priority Demo");
