#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "mem_ioctl.h"

#define FIRST_MINOR 0
#define MINOR_CNT 1

static dev_t dev;
static struct cdev c_dev;
static int store_size;
static char *c;
static int store_len;

static int my_open(struct inode *i, struct file *f)
{
	return 0;
}
static int my_close(struct inode *i, struct file *f)
{
	return 0;
}
static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	int copy_len;

	if (*off < store_len)
	{
		copy_len = (len < (store_len - *off)) ? len : (store_len - *off);
		if (copy_to_user(buf, &c[*off], copy_len))
		{
			return -EFAULT;
		}
		(*off) += copy_len;
		return copy_len;
	}
	else
	{
		return 0;
	}
}
static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	store_len = (len < store_size) ? len : store_size;
	if (copy_from_user(c, &buf[len - store_len], store_len))
	{
		store_len = 0;
		return -EFAULT;
	}
	return len;
}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int my_ioctl(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg)
#else
static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
#endif
{
	switch (cmd)
	{
		case MEM_SET_STORE_SIZE:
			//printk(KERN_INFO "Store size requested (%d): %d\n", sizeof(arg), (int)arg);
			if (store_size)
			{
				store_len = 0;
				kfree(c);
			}
			store_size = arg;
			if ((c = kmalloc(store_size, GFP_KERNEL)) == NULL)
			{
				return -ENOMEM;
			}
			break;
		case MEM_GET_STORE_SIZE:
			if (copy_to_user((int *)arg, &store_size, sizeof(int)))
			{
				return -EFAULT;
			}
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static struct file_operations driver_fops =
{
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.read = my_read,
	.write = my_write,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	.ioctl = my_ioctl
#else
	.unlocked_ioctl = my_ioctl
#endif
};

static int __init mem_ioctl_init(void)
{
	int ret;

	if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "mem_ioctl")) < 0)
	{
		return ret;
	}

	cdev_init(&c_dev, &driver_fops);

	if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
	{
		return ret;
	}
	
	return 0;
}

static void __exit mem_ioctl_exit(void)
{
	if (store_size)
	{
		store_len = 0;
		kfree(c);
		store_size = 0;
	}

	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
}

module_init(mem_ioctl_init);
module_exit(mem_ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Memory based Char Driver");
