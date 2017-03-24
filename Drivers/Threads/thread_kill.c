#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/signal.h>
#include <linux/delay.h>

static struct task_struct *thread_st;
// Function executed by kernel thread
static int thread_fn(void *unused)
{
	// Allow the SIGKILL signal
	allow_signal(SIGKILL);
	while (!kthread_should_stop())
	{
		printk(KERN_INFO "Thread Running\n");
		ssleep(5);
		if (signal_pending(current))
			break;
	}
	printk(KERN_INFO "Thread Stopping\n");
	thread_st = NULL;
	do_exit(0);
}
// Module Initialization
static int __init init_thread(void)
{
	printk(KERN_INFO "Creating Thread\n");
	//Create the kernel thread with name 'mythread'
	thread_st = kthread_run(thread_fn, NULL, "mythread");
	if (thread_st)
		printk("Thread Created successfully\n");
	else
		printk(KERN_INFO "Thread creation failed\n");
	return 0;
}
// Module Exit
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
MODULE_DESCRIPTION("Thread Complete Demo");
