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
#include <linux/interrupt.h>

#define BUTTON	72
static char flag = 'n';
static int irq_line;

static wait_queue_head_t wq_head;

static struct task_struct *thread_st;
static int minor_start = 0;
static int minor_cnt = 1;
static dev_t dev;
static struct cdev cdev;
static struct class *class;
static struct semaphore read_mutex;
static char val;

static irqreturn_t button_irq_handler(int irq, void *data)
{
	printk("GPIO value is %d\n", (val = gpio_get_value(BUTTON)));
	wake_up_interruptible(&wq_head);
	up(&read_mutex);
	flag = 'y';
	return IRQ_HANDLED;
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
	int irq_req_res;

	printk(KERN_DEBUG "Inserting Interrupt select module\n"); 

	/* Registering device */
	result = alloc_chrdev_region(&dev, minor_start, minor_cnt, "gpio");

	if (result < 0)
	{
		printk(KERN_ERR "cannot obtain major number");
		return result;
	}
	else
	{
		printk(KERN_INFO "Obtained %d as select major number\n", MAJOR(dev));
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
	gpio_request_one(BUTTON, GPIOF_IN, "button");
	if ( (irq_line = gpio_to_irq(BUTTON)) < 0)
	{
		printk(KERN_ALERT "Gpio %d cannot be used as interrupt",BUTTON);
		return -EINVAL;
	}

	if ( (irq_req_res = request_irq(irq_line, button_irq_handler, 
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "gpio_change_state", NULL)) < 0) 
	{
		printk(KERN_ERR "Keypad: registering irq failed\n");
		return -EINVAL;
	}
	init_waitqueue_head(&wq_head);	
	sema_init(&read_mutex, 0);
	return 0;
}

static void __exit cleanup_gpio(void)
{
	int ret = -1;
	free_irq(irq_line, NULL);
	printk("Cleaning Up\n");
	if (thread_st)
		ret = kthread_stop(thread_st);
	if(!ret)
		printk(KERN_INFO "Thread stopped");
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
MODULE_DESCRIPTION("Interrupt with Select Demo");
