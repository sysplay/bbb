//#define USE_SPINLOCK // TODO 2: Uncomment

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/signal.h>
#include <linux/delay.h>
#include <linux/timer.h>
#ifdef USE_SPINLOCK
#include <linux/spinlock.h>
#else
#include <linux/mutex.h>
#endif
#include <asm/uaccess.h>

#define FIRST_MINOR 0
#define MINOR_CNT 1

#ifdef USE_SPINLOCK
#define LOCK spin_lock(&s)
#define UNLOCK spin_unlock(&s)
#else
#define LOCK mutex_lock(&m)
#define UNLOCK mutex_unlock(&m)
#endif

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

static struct task_struct *tb_def_thread_st;
static struct task_struct *tb_thread_st;
static struct timer_list tb_timer;
static int tb_count = 0;
#ifdef USE_SPINLOCK
spinlock_t s;
#else
static DEFINE_MUTEX(m);
#endif

void tb_timer_callback(unsigned long data)
{
	//LOCK; // TODO 1: Uncomment
	tb_count++;
	//UNLOCK; // TODO 1: Uncomment
	printk(KERN_INFO "Timer call-back called (%lu) - %d times.\n", jiffies, tb_count);
	mod_timer(&tb_timer, jiffies + msecs_to_jiffies(2000));
}

static int tb_thread_fn(void *param)
{
	char *label = ((param) ? (char *)(param) : "Trigger");
	int secs = ((param) ? 7 : 5);

	if (!param)
	{
		printk(KERN_INFO "Timer module installing\n");
		setup_timer(&tb_timer, tb_timer_callback, 0);
		printk(KERN_INFO "Starting timer to fire in 2s (%lu)\n", jiffies);
		if (mod_timer(&tb_timer, jiffies + msecs_to_jiffies(2000)))
			printk(KERN_ERR "Error in mod_timer\n");
	}

	// Allow the SIGKILL signal
	allow_signal(SIGKILL);
	while (!kthread_should_stop())
	{
		LOCK;
		pr_debug(KERN_INFO "%s Thread Running w/ count as %d\n", label, tb_count);
		UNLOCK;
		ssleep(secs);
		if (signal_pending(current))
			break;
	}
	printk(KERN_INFO "%s Thread Stopping\n", label);
	if (param)
	{
		tb_def_thread_st = NULL;
	}
	else
	{
		tb_thread_st = NULL;
	}
	do_exit(0);
}

static int tb_open(struct inode *i, struct file *f)
{
	return 0;
}
static int tb_close(struct inode *i, struct file *f)
{
	return 0;
}

static ssize_t tb_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	if (tb_thread_st)
		return -EBUSY;

	printk(KERN_INFO "Creating Thread\n");
	//Create the kernel thread with name 'tb_thread'
	tb_thread_st = kthread_run(tb_thread_fn, NULL, "tb_thread");
	if (tb_thread_st)
		printk("Thread Created successfully\n");
	else
		printk(KERN_INFO "Thread creation failed\n");

	return 0;
}
static ssize_t tb_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	if (!tb_thread_st)
		return -EINVAL;

	printk("Closing Down\n");

	if (del_timer(&tb_timer))
	{
		printk(KERN_INFO "The timer is still in use\n");
	}

	kthread_stop(tb_thread_st);
	printk(KERN_INFO "Thread stopped\n");

	return len;
}

static struct file_operations driver_fops =
{
	.owner = THIS_MODULE,
	.open = tb_open,
	.release = tb_close,
	.read = tb_read,
	.write = tb_write
};

static int __init tb_debug_init(void)
{
	int ret;
	struct device *dev_ret;

	if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "thread_builtin")) < 0)
	{
		return ret;
	}

	cdev_init(&c_dev, &driver_fops);

	if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
	{
		unregister_chrdev_region(dev, MINOR_CNT);
		return ret;
	}

	if (IS_ERR(cl = class_create(THIS_MODULE, "thread")))
	{
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(cl);
	}
	if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "tb")))
	{
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(dev_ret);
	}

#ifdef USE_SPINLOCK
	spin_lock_init(&s);
#endif

	printk(KERN_INFO "Creating Default Thread\n");
	//Create the kernel thread with name 'tb_def_thread'
	tb_def_thread_st = kthread_run(tb_thread_fn, "Default", "tb_def_thread");
	if (tb_def_thread_st)
		printk("Default Thread Created successfully\n");
	else
		printk(KERN_INFO "Default Thread creation failed\n");

	return 0;
}

static void __exit tb_debug_exit(void)
{
	printk("Cleaning Up thru exit\n");

	if (tb_thread_st)
	{
		if (del_timer(&tb_timer))
		{
			printk(KERN_INFO "The timer is still in use in exit\n");
		}

		kthread_stop(tb_thread_st);
		printk(KERN_INFO "Thread stopped thru exit\n");
	}

	if (tb_def_thread_st)
	{
		kthread_stop(tb_def_thread_st);
		printk(KERN_INFO "Default Thread stopped\n");
	}

	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
}

module_init(tb_debug_init);
module_exit(tb_debug_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anil Kumar Pugalia <anil@sysplay.in>");
MODULE_DESCRIPTION("Thread BuiltIn Debug Driver");
