#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>  // for threads
#include <linux/sched.h>  // for task_struct
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>

static struct task_struct *thread1,*thread2;
static spinlock_t my_lock;

int thread_fn1(void *dummy) 
{
	int i;

	allow_signal(SIGKILL);
	printk("Entering in a Lock\n");
	spin_lock(&my_lock);
	for (i = 0; i < 20; i++)
	{
		printk("In Spin Lock\n");
		ssleep(1);
		if (signal_pending(current))
			break;
	}	
	spin_unlock(&my_lock);
	printk("Exiting the Lock\n");
	thread1 = NULL;
	do_exit(0);
}

int thread_fn2(void *dummy) 
{
	int ret=0;
	allow_signal(SIGKILL);
	msleep(100);
	ret=spin_trylock(&my_lock);
	if(!ret) 
	{
		printk(KERN_INFO "Unable to hold lock");
		return 0;
	} 
	else 
	{
		printk(KERN_INFO "Lock acquired");
		spin_unlock(&my_lock);
		return 0;
	}
	thread2 = NULL;
	do_exit(0);
}


int spinlock_init(void) 
{
	spin_lock_init(&my_lock);
	thread1 = kthread_create(thread_fn1, NULL, "thread1");
	if(thread1)
	{
		printk("Thread 1 woken up\n");
		wake_up_process(thread1);
	}
	thread2 = kthread_create(thread_fn2, NULL, "thread2");

	if(thread2)
	{
		printk("Thread 2 woken up\n");
		wake_up_process(thread2);
	}
	do_exit(0);
}


void spinlock_cleanup(void) 
{
	if (thread1 != NULL)
	{
		kthread_stop(thread1);
		printk("Thread 1 Stopped\n");
	}
	if (thread2 != NULL)
	{
		kthread_stop(thread2);
		printk("Thread 2 Stopped\n");
	}
}

module_init(spinlock_init);
module_exit(spinlock_cleanup);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Spin Lock Demo");
