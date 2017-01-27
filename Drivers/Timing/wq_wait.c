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
	wait_queue_head_t wq_head;
	init_waitqueue_head(&wq_head);
	
	while (!kthread_should_stop()) 
	{
		j1 = jiffies;
		wait_event_interruptible_timeout(wq_head, 0, delay);
		j2 = jiffies;
		printk("Jit Start = %lu\t Jit End = %lu\n", j1, j2);
	}
	printk(KERN_INFO "Thread Stopping\n");
    do_exit(0);
}


static int __init init_thread(void)
{
	printk(KERN_INFO "Creating Thread\n");
	thread_st = kthread_create(thread_fn, NULL, "mythread");
	if (thread_st)
		wake_up_process(thread_st);
	else
		printk(KERN_INFO "Thread creation failed\n");

	return 0;
}

static void __exit cleanup_thread(void)
{
	int ret;
	printk("Cleaning Up\n");
	ret = kthread_stop(thread_st);
	if(!ret)
		printk(KERN_INFO "Thread stopped");
}

module_init(init_thread);
module_exit(cleanup_thread);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Wait Queue Demo");
