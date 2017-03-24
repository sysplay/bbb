#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>

static struct kprobe kp;
static unsigned long base = 0xbf000000;
module_param(base, ulong, 0);

static int pre(struct kprobe *p, struct pt_regs *regs)
{
	printk(KERN_INFO "kprobe: Pre handler: 0x%p, r2: 0x%lx\n", p->addr, regs->uregs[2]);
	return 0;
}

static void post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	regs->uregs[2]--;
	printk(KERN_INFO "kprobe: Post handler: 0x%p, r2: 0x%lx\n", p->addr, regs->uregs[2]);
}

static int fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(KERN_INFO "kprobe: Fault handler: 0x%p, Trap: %d\n", p->addr, trapnr);
	return 0;
}

static int __init kprobe_init(void)
{
	int ret;

	kp.pre_handler = pre;
	kp.post_handler = post;
	kp.fault_handler = fault;

	//kp.symbol_name = "my_read";
	//kp.offset = 0x134;
	kp.addr = (void *)base + 0x134;

	if ((ret = register_kprobe(&kp)) < 0)
	{
		printk(KERN_ERR "kprobe: Registration failed\n");
	}
	else
	{
		printk(KERN_INFO "kprobe: Planted @ 0x%p\n", kp.addr);
	}
	return ret;
}

static void __exit kprobe_exit(void)
{
	unregister_kprobe(&kp);
	printk(KERN_INFO "kprobe: Unregistered\n");
}

module_init(kprobe_init);
module_exit(kprobe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("KProbe Demo Driver");
