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

static struct task_struct *thread_st;
static DECLARE_WAIT_QUEUE_HEAD(wq);
static int flag = 0;

static int thread_fn(void *unused)
{
	printk("Going to Sleep\n");
	wait_event_interruptible(wq, flag != 0);
	flag = 0;
	printk("Woken Up\n");
	thread_st = NULL;
    do_exit(0);
}


static int __init init_thread(void)
{
	printk(KERN_INFO "Creating Thread 1\n");
	thread_st = kthread_run(thread_fn, NULL, "mythread");
	ssleep(10);
	printk("Waking Up Thread\n");
	flag = 1;
	wake_up_interruptible(&wq);
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
MODULE_DESCRIPTION("Wait Queue Demo");
