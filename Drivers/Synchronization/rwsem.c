#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h> // required for various structures related to files liked fops.
#include <asm/uaccess.h> // required for copy_from and copy_to user functions
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/delay.h>

#define FIRST_MINOR 0
#define MINOR_CNT 1

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
struct task_struct *task;
static struct rw_semaphore rwsem;


int open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Inside open \n");
	task = current;
	return 0;
}

int release(struct inode *inode, struct file *filp) 
{
	printk (KERN_INFO "Inside close \n");
	return 0;
}

ssize_t read(struct file *filp, char *buff, size_t count, loff_t *offp) 
{

	printk("Inside read \n");
	down_read(&rwsem);
	printk(KERN_INFO "Got the Semaphore in Read\n");
	printk("Going to Sleep\n");
	ssleep(30);
	up_read(&rwsem);
	return 0;
}

ssize_t write(struct file *filp, const char *buff, size_t count, loff_t *offp) 
{   
	printk(KERN_INFO "Inside write \n");
	down_write(&rwsem);
	printk(KERN_INFO "Got the Semaphore in Write\n");
	up_write(&rwsem);
	return count;
}

struct file_operations fops = {
	read:        read,
	write:        write,
	open:         open,
	release:    release
};


int rw_sem_init (void) 
{
	if (alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "SCD") < 0)
	{
		return -1;
	}
	printk("Major Nr: %d\n", MAJOR(dev));

	cdev_init(&c_dev, &fops);

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
	init_rwsem(&rwsem);

	return 0;
}

void rw_sem_cleanup(void) 
{
	printk(KERN_INFO " Inside cleanup_module\n");
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
}

module_init(rw_sem_init);
module_exit(rw_sem_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Reader Writer Semaphore Demo");
