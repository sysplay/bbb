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
#include <linux/poll.h>

#define BUTTON	72
static char flag = 'n';

static wait_queue_head_t wq_head;

static struct task_struct *thread_st;
static int minor_start = 0;
static int minor_cnt = 1;
static dev_t dev;
static struct cdev cdev;
static struct class *class;
static struct semaphore read_mutex;
static char val;

static int thread_fn(void *unused)
{
	printk(KERN_INFO "Thread Running\n");
	allow_signal(SIGKILL);
	while (!kthread_should_stop()) 
	{
		if (!gpio_get_value(BUTTON)) 
		{
			printk("Button Pressed\n");
			wake_up_interruptible(&wq_head);
			up(&read_mutex);
			val = '1';
			flag = 'y';
		}
		ssleep(2);
		if (signal_pending(current))
			break;
	}
	printk(KERN_INFO "Thread Stopping\n");
	thread_st = NULL;
	do_exit(0);
}

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
	printk("In Poll- GPIO\n");
	poll_wait(f, &wq_head, wait);
	printk("Out Poll- GPIO\n");

	return (flag == 'y' ? (POLLIN | POLLRDNORM) : 0);
}

static struct file_operations my_fops = {
	.read = my_read,
	.poll = my_poll,
};

static int __init init_gpio(void)
{
	int result;
	printk(KERN_INFO "Creating Thread\n");
	gpio_request_one(BUTTON, GPIOF_IN, "button");
	thread_st = kthread_create(thread_fn, NULL, "mythread");
	if (thread_st)
		wake_up_process(thread_st);
	else {
		printk(KERN_INFO "Thread creation failed\n");
		return -1;
	}

	printk(KERN_DEBUG "Inserting select module\n"); 

	/* Registering device */
	result = alloc_chrdev_region(&dev, minor_start, minor_cnt, "gpio");

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

	class = class_create(THIS_MODULE, "gpiodrv");
	if (IS_ERR(class))
	{
		printk(KERN_ERR "Cannot create class\n");
		cdev_del(&cdev);
		unregister_chrdev_region(dev, minor_cnt);
		return -1;
	}

	if (IS_ERR(device_create(class, NULL, dev, NULL, "gpio%d", 0)))
	{
		printk(KERN_ERR "Gpio: cannot create device\n");
		class_destroy(class);
		cdev_del(&cdev);
		unregister_chrdev_region(dev, minor_cnt);
		return -1;
	}
	init_waitqueue_head(&wq_head);	
	sema_init(&read_mutex, 0);
	return 0;
}

static void __exit cleanup_gpio(void)
{
	printk("Cleaning Up\n");
	if (thread_st != NULL)
	{
		kthread_stop(thread_st);
		printk(KERN_INFO "Thread stopped");
	}
	device_destroy(class, dev);
	class_destroy(class);
	cdev_del(&cdev);
	/* Freeing the major number */
	unregister_chrdev_region(dev, minor_cnt);

}

module_init(init_gpio);
module_exit(cleanup_gpio);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("GPIO Select Demo");
