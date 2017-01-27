#include <linux/module.h>
#include <linux/kernel.h>

static int __init mfd_init(void) /* Constructor */
{
	printk(KERN_INFO "mfd registered\n");
	return 0;
}

static void __exit mfd_exit(void) /* Destructor */
{
	printk(KERN_INFO "mfd unregistered\n");
}

module_init(mfd_init);
module_exit(mfd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("My First Driver");
