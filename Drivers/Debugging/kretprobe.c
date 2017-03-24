#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>

static struct kretprobe krp;
static unsigned long base = 0xbf000000;
module_param(base, ulong, 0);

static int entry(struct kretprobe_instance *pi, struct pt_regs *regs)
{
	printk(KERN_INFO "kretprobe: Filling kretprobe instance data\n");
	*(char *)(pi->data) = 'K';
	return 0;
}

static int handler(struct kretprobe_instance *pi, struct pt_regs *regs)
{
	printk(KERN_INFO "kretprobe: Recovering kretprobe instance data: %c\n", *(char *)(pi->data));
	if (regs->uregs[0] == -1)
	{
		regs->uregs[0] = 0;
		printk(KERN_INFO "kretprobe: Fixed the -1 return value to 0\n");
	}
	return 0;
}

static int __init kretprobe_init(void)
{
	int ret;

	krp.handler = handler;
	krp.entry_handler = entry;
	krp.maxactive = 1;
	krp.data_size = sizeof(char);
	krp.kp.addr = (void *)base + 0xAC;

	if ((ret = register_kretprobe(&krp)) < 0)
	{
		printk(KERN_ERR "kretprobe: Registration failed\n");
	}
	else
	{
		printk(KERN_INFO "kretprobe: Planted @ 0x%p\n", krp.kp.addr);
	}
	return ret;
}

static void __exit kretprobe_exit(void)
{
	unregister_kretprobe(&krp);
	printk(KERN_INFO "kretprobe: Unregistered\n");
	printk(KERN_INFO "kretprobe: Missed probing %d instances\n", krp.nmissed);
}

module_init(kretprobe_init);
module_exit(kretprobe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("KProbe Demo Driver");
