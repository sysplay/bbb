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
#include <linux/proc_fs.h>

#define FIRST_MINOR 0
#define MINOR_CNT 1

struct proc_dir_entry *my_proc_file;
char flag = 'n';
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
static struct task_struct *sleeping_task;

int open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Inside open \n");
	sleeping_task = current;
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
	printk("Scheduling Out\n");
	sleeping_task = current;
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
	printk(KERN_INFO "Woken Up\n");
	return 0;
}

ssize_t write(struct file *filp, const char *buff, size_t count, loff_t *offp) 
{   
	return 0;
}

int write_proc(struct file *file,const char *buffer, size_t count, loff_t *off)
{
	int ret = 0;
	printk(KERN_INFO "procfile_write /proc/wait called");
	ret = __get_user(flag,buffer);
	printk(KERN_INFO "%c",flag);
	wake_up_process(sleeping_task);
	return count;
}

struct file_operations p_fops = {
	.write = write_proc
};

struct file_operations pra_fops = {
	read:        read,
	write:        write,
	open:         open,
	release:    release
};


struct cdev *kernel_cdev;

static int create_new_proc_entry(void)
{
	proc_create("wait",0,NULL,&p_fops);
	return 0;
}

int schd_init (void) 
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

	create_new_proc_entry();
	return 0;
}


void schd_cleanup(void) 
{
	printk(KERN_INFO " Inside cleanup_module\n");
	remove_proc_entry("wait",NULL);
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
}

module_init(schd_init);
module_exit(schd_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Waiting Process Demo");
