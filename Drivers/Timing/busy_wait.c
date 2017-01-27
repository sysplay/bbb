#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>

int delay = HZ;
module_param(delay, int, 0);

static struct task_struct *thread_st;

static int thread_fn(void *unused)
{
	unsigned long j1, j2;

	allow_signal(SIGKILL);
	while (!kthread_should_stop()) 
	{
		j1 = jiffies;
		j2 = j1 + delay;
		while (time_before(jiffies, j2))
			cpu_relax();
		j2 = jiffies;
		printk("Jit Start = %lu\t Jit End = %lu\n", j1, j2);
		if (signal_pending(current))
			break;
	}
	printk(KERN_INFO "Thread Stopping\n");
	thread_st = NULL;
	do_exit(0);
}

static int __init init_thread(void)
{
	printk(KERN_INFO "Creating Thread\n");
	thread_st = kthread_run(thread_fn, NULL, "mythread");
	return 0;
}

static void __exit cleanup_thread(void)
{
	printk("Cleaning Up\n");
	if (thread_st != NULL)
	{
		kthread_stop(thread_st);
		printk(KERN_INFO "Thread stopped");
	}
}

module_init(init_thread);
module_exit(cleanup_thread);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Busy Wait Demo");
