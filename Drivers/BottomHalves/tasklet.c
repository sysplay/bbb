#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

char my_tasklet_data[] = "my_tasklet_function_was_called";

void my_tasklet_function(unsigned long data)
{
	printk("%s\n", (char *)data);
	return;
}

DECLARE_TASKLET(my_tasklet, my_tasklet_function, (unsigned long)&my_tasklet_data);

int init_module(void)
{
	tasklet_schedule(&my_tasklet);
	return 0;
}

void cleanup_module(void)
{
	tasklet_kill(&my_tasklet);
	return;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Tasklet Demo");
