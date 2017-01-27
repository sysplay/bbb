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

#define FIRST_MINOR 0
#define MINOR_CNT 1

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

static struct task_struct *thread_st[10];
static DECLARE_WAIT_QUEUE_HEAD(wq);
static int flag = 0;
static char c;
static struct semaphore sem;
static int a[10];

static int thread_fn(void *data)
{
	allow_signal(SIGKILL);
	if (data == NULL)
	{
		printk("Data is NULL\n");
		return -1;
	}
	printk("Data is %d\n", *(int *)data);

	if (down_interruptible(&sem))
		return -ERESTARTSYS;

	while (flag != 'y')
	{
		up(&sem);
		printk("Thread %d going to Sleep\n", *(int *)data);
		if (wait_event_interruptible(wq, flag == 'y'))
			return -ERESTARTSYS;

		printk("Thread %d Woken Up\n", *(int *)data);
		if (down_interruptible(&sem))
			return -ERESTARTSYS;
	}
	ssleep(1);
	flag = 'n';
	up(&sem);

	printk("Data read by %d thread is %c\n", *(int *)data, c);
	thread_st[*(int *)data] = NULL;
	do_exit(0);
}

ssize_t write(struct file *filp, const char *buff, size_t count, loff_t *offp) {

	printk(KERN_INFO "Inside write \n");
	if (copy_from_user(&flag, buff, 1)) 
	{
		printk("copy_from_user failed\n");
		return -EFAULT;
	}
	wake_up_interruptible(&wq);
	return count;
}       

int open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Inside open \n");
	return 0;
}

int release(struct inode *inode, struct file *filp) 
{
	printk (KERN_INFO "Inside close \n");
	return 0;
}

struct file_operations pra_fops = {
	write:        write,
	open:         open,
	release:    release
};


static int __init wqt_init(void)
{
	int i = 0;
	char name[20];

	sema_init(&sem, 1);
	for (i = 0; i < 10; i++) 
	{
		a[i] = i;
		sprintf(name, "mythread%d", a[i]);
		printk(KERN_INFO "Creating Thread- %d\n", a[i]);
		thread_st[i] = kthread_run(thread_fn, &a[i], name);
	}
	printk("Waking Up Thread\n");
	flag = 1;
	wake_up_interruptible(&wq);

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

static void __exit wqt_exit(void)
{
	int i;
	printk("Cleaning Up\n");

	for (i = 0; i < 10; i++) 
	{
		if (thread_st[i] != NULL)
			kthread_stop(thread_st[i]);
	}
	printk(KERN_INFO " Inside cleanup_module\n");
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
}

module_init(wqt_init);
module_exit(wqt_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Wait Queue Thunder Demo");
