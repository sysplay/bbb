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
#include <linux/gpio.h>
#include <linux/timer.h>

#define GPIO	56
#define DELAY	HZ

void timer_fn(unsigned long);
static struct task_struct *thread_st;
static wait_queue_head_t wq_head;
static char flag = 0;
static struct timer_list my_timer = TIMER_INITIALIZER(timer_fn, 0, 0);

void timer_fn(unsigned long data)
{
	int ret;
	flag = 1;
	printk("\nTimer Fired\n");
	wake_up(&wq_head);
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));
	if (ret) 
		printk("\nError in mod_timer\n");
}

static int thread_fn(void *unused)
{
	static char state = 0;
	while (!kthread_should_stop()) 
	{
		printk("\nWaiting\n");
		wait_event_interruptible(wq_head, flag == 1);
		state ^= 1;
		printk("\nWoken Up\n");
		gpio_set_value(GPIO, state);
		flag = 0;
	}
	printk(KERN_INFO "Thread Stopping\n");
    do_exit(0);
}


static int __init init_thread(void)
{
	init_waitqueue_head(&wq_head);

	printk(KERN_INFO "Creating Thread\n");
	thread_st = kthread_create(thread_fn, NULL, "mythread");
	if (thread_st)
		wake_up_process(thread_st);
	else
		printk(KERN_INFO "Thread creation failed\n");
	/* Start the timer */
	mod_timer(&my_timer, jiffies + DELAY);

	return 0;
}

static void __exit cleanup_thread(void)
{
	int ret;
	printk("Cleaning Up\n");
	ret = kthread_stop(thread_st);
	if(!ret)
		printk(KERN_INFO "Thread stopped");
	del_timer(&my_timer);
}

module_init(init_thread);
module_exit(cleanup_thread);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Kernel Timers Demo");
