#include <linux/module.h>
#include <linux/kernel.h>

static int __init init_kmod(void)
{
	printk("Kernel Module Registered\n");
	return 0;
}

static void __exit cleanup_kmod(void)
{
	printk("Kernel Module Unregistered");
}

module_init(init_kmod);
module_exit(cleanup_kmod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Kernel Module");
