#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>		/* printk() */
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/moduleparam.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>

#define DELAY	HZ

void timer_fn(unsigned long);
static int minor_start = 0;
static int minor_cnt = 1;
static dev_t dev;
static struct cdev cdev;
static struct class *class;

static wait_queue_head_t wq_head;
static char flag = 'n';
static struct semaphore read_mutex;
static struct timer_list my_timer = TIMER_INITIALIZER(timer_fn, 0, 0);
static char val;

static ssize_t my_read(struct file *f, char __user *buf, size_t cnt, loff_t *off)
{
	printk("In Read\n");
	if (down_interruptible(&read_mutex)) 
	{
		printk("Semaphore Failed\n");
		return -1;
	}

	if (copy_to_user(buf, &val, 1))
	{
		return -EINVAL;
	}
	printk("Sent the status\n");
	flag = 'n';
	return 1;
}

static unsigned int my_poll(struct file *f, poll_table *wait)
{
	printk("In Poll\n");
	poll_wait(f, &wq_head, wait);
	printk("Out Poll\n");

	return (flag == 'y' ? (POLLIN | POLLRDNORM) : 0);
}

static struct file_operations my_fops = {
	.read = my_read,
	.poll = my_poll,
};

void timer_fn(unsigned long data)
{
	int ret;
	printk("\nTimer Fired\n");
	flag = 'y';
	wake_up_interruptible(&wq_head);
	val = '1';	
	up(&read_mutex);
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));
	if (ret) 
		printk("\nError in mod_timer\n");
}


static int __init my_init(void)
{
	int result;

	printk(KERN_DEBUG "Inserting select module\n"); 

	/* Registering device */
	result = alloc_chrdev_region(&dev, minor_start, minor_cnt, "select");

	if (result < 0)
	{
		printk(KERN_ERR "cannot obtain major number");
		return result;
	}
	else
	{
		printk(KERN_DEBUG "Obtained %d as select major number\n", MAJOR(dev));
	}

	cdev_init(&cdev, &my_fops);
	if ((result = cdev_add(&cdev, dev, minor_cnt)) < 0)
	{
		printk(KERN_ERR "cannot add functions\n");
		/* Freeing the major number */
		unregister_chrdev_region(dev, minor_cnt);
		return result;
	}

	class = class_create(THIS_MODULE, "select");
	if (IS_ERR(class))
	{
		printk(KERN_ERR "Cannot create class\n");
		cdev_del(&cdev);
		unregister_chrdev_region(dev, minor_cnt);
		return -1;
	}

	if (IS_ERR(device_create(class, NULL, dev, NULL, "mychar%d", 0)))
	{
		printk(KERN_ERR "Select: cannot create device\n");
		class_destroy(class);
		cdev_del(&cdev);
		unregister_chrdev_region(dev, minor_cnt);
		return -1;
	}
	sema_init(&read_mutex, 0);
	init_waitqueue_head(&wq_head);	

	/* Start the timer */
	mod_timer(&my_timer, jiffies + DELAY);

	return 0;
}

static void __exit my_cleanup(void)
{
	int ret;
	device_destroy(class, dev);
	class_destroy(class);
	cdev_del(&cdev);
	/* Freeing the major number */
	unregister_chrdev_region(dev, minor_cnt);

	ret = del_timer(&my_timer);
	if (ret)
		printk("The timer is still in use...\n");

	printk(KERN_INFO "Removing select module\n");
}

module_init(my_init);
module_exit(my_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Timer Select Demo");
