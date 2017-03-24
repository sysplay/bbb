#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>

static struct jprobe jp;
static unsigned long base = 0xbf000000;
module_param(base, ulong, 0);

static ssize_t hack_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	printk(KERN_INFO "jprobe: Hacked into read\n");
	printk(KERN_INFO "jprobe: Received Buf: 0x%p, Len: %d, Off: %Ld\n", buf, len, *off);
	if (*off > 1)
	{
		*off = 1;
		printk(KERN_INFO "jprobe: Modified offset to 1\n");
	}
	/* Always return with this */
	jprobe_return();
	return 0;
}

static int __init jprobe_init(void)
{
	int ret;

	jp.entry = hack_read;
	jp.kp.addr = (void *)base + 0xAC;

	if ((ret = register_jprobe(&jp)) < 0)
	{
		printk(KERN_ERR "jprobe: Registration failed\n");
	}
	else
	{
		printk(KERN_INFO "jprobe: Planted @ 0x%p with handler addr 0x%p\n", jp.kp.addr, jp.entry);
	}
	return ret;
}

static void __exit jprobe_exit(void)
{
	unregister_jprobe(&jp);
	printk(KERN_INFO "jprobe: Unregistered\n");
}

module_init(jprobe_init);
module_exit(jprobe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("JProbe Demo Driver");
