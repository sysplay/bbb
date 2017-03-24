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
#include <linux/semaphore.h>
#include <linux/proc_fs.h>

#define FIRST_MINOR 0
#define MINOR_CNT 1

char flag = 'n';
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

DECLARE_WAIT_QUEUE_HEAD(wq_head);

int open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Inside open \n");
	return 0;
}


int release(struct inode *inode, struct file *filp) 
{
	printk(KERN_INFO "Inside close \n");
	return 0;
}

ssize_t read(struct file *filp, char *buff, size_t count, loff_t *offp) 
{
	DEFINE_WAIT(wait);
	printk("Inside read \n");
	printk("Preparing to sleep\n");
	prepare_to_wait(&wq_head, &wait, TASK_INTERRUPTIBLE);
	schedule();
	finish_wait(&wq_head, &wait);
	return 0;
}

ssize_t write(struct file *filp, const char *buff, size_t count, loff_t *offp) 
{   
	printk(KERN_INFO "Inside write \n");
	wake_up(&wq_head);
	return count;
}

struct file_operations pra_fops = {
	read:        read,
	write:        write,
	open:         open,
	release:    release
};

int mwait_init (void) 
{
	if (alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "SCD") < 0)
	{
		return -1;
	}
	printk("Major Nr: %d\n", MAJOR(dev));

	cdev_init(&c_dev, &pra_fops);

	if (cdev_add(&c_dev, dev, MINOR_CNT) == -1)
	{
		unregister_chrdev_region(dev, MINOR_CNT);
		return -1; 
	}

	if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
	{
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return -1;
	}
	if (IS_ERR(device_create(cl, NULL, dev, NULL, "mychar%d", 0)))
	{
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return -1;
	}

	return 0;
}


void mwait_cleanup(void) 
{
	printk(KERN_INFO " Inside cleanup_module\n");
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
}

module_init(mwait_init);
module_exit(mwait_cleanup);

MODULE_LICENSE("GPL");   
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Manual Wait Demo");
