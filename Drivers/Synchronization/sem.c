#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h> // required for various structures related to files liked fops.
#include <asm/uaccess.h> // required for copy_from and copy_to user functions
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>

#define FIRST_MINOR 0
#define MINOR_CNT 1

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
struct task_struct *task;
static struct semaphore sem;

int open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Inside open\n");
	task = current;                 // Saving the task_struct for future use.
	if(down_interruptible(&sem)) 
	{
		printk(KERN_INFO " could not hold semaphore");
		return -1;
	}
	printk(KERN_INFO "State after = %ld\n", task->state); // printing the state of user process
	return 0;
}

int release(struct inode *inode, struct file *filp) 
{
	printk (KERN_INFO "Inside close \n");
	printk(KERN_INFO "Releasing semaphore\n");
	up(&sem);
	return 0;
}

ssize_t read(struct file *filp, char *buff, size_t count, loff_t *offp) 
{
	printk("Inside read \n");
	return 0;
}

ssize_t write(struct file *filp, const char *buff, size_t count, loff_t *offp) 
{   

	printk(KERN_INFO "Inside write \n");
	return count;
}

int hold(struct file *filp,char *buf,size_t count,loff_t *offp)
{
	int len=0;
	if (down_interruptible(&sem))   // holding the semaphore
		return -ERESTARTSYS;
	printk(KERN_INFO "Inside hold\n");
	return len;
}

int remove(struct file *file,const char *buffer, size_t count, loff_t *off)
{
	/* according to linux/sched.h the value of state and their meanings are
	 * -1 unrunnable, 0 runnable, >0 stopped
	 */
	printk(KERN_INFO "State before= %ld\n", task->state);  // printing the state of user process
	up(&sem);
	printk(KERN_INFO "Inside remove\n");

	return count;
}
struct file_operations p_fops = {
	.read = hold,
	.write = remove,
};


void create_new_proc_entry(void)
{
	proc_create("hold",0,NULL,&p_fops);
}

struct file_operations fops = {
	read:        read,
	write:        write,
	open:         open,
	release:    release
};


int semdemo_init (void) 
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

	sema_init(&sem, 1);

	create_new_proc_entry();
	return 0;
}

void semdemo_cleanup(void) 
{
	printk(KERN_INFO " Inside cleanup_module\n");
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
	remove_proc_entry("hold",NULL);
}

module_init(semdemo_init);
module_exit(semdemo_cleanup);
MODULE_LICENSE("GPL");   
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Semaphore Demo");
