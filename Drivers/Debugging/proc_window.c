#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
#else
#include <linux/fs.h>
#include <linux/seq_file.h>
#endif
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
#define STR_PRINTF_RET(len, str, args...) len += sprintf(page + len, str, ## args)
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(4,3,0))
#define STR_PRINTF_RET(len, str, args...) len += seq_printf(m, str, ## args)
#else
#define STR_PRINTF_RET(len, str, args...) seq_printf(m, str, ## args)
#endif

static struct proc_dir_entry *parent, *file, *link;
static int state = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
static int time_read(char *page, char **start, off_t off, int count, int *eof, void *data)
#else
static int time_read(struct seq_file *m, void *v)
#endif
{
	int len = 0, val;
	unsigned long act_jiffies;

	STR_PRINTF_RET(len, "state = %d\n", state);
	act_jiffies = jiffies - INITIAL_JIFFIES;
	val = jiffies_to_msecs(act_jiffies);
	switch (state)
	{
		case 0:
			STR_PRINTF_RET(len, "time = %ld jiffies\n", act_jiffies);
			break;
		case 1:
			STR_PRINTF_RET(len, "time = %d msecs\n", val);
			break;
		case 2:
			STR_PRINTF_RET(len, "time = %ds %dms\n",
					val / 1000, val % 1000);
			break;
		case 3:
			val /= 1000;
			STR_PRINTF_RET(len, "time = %02d:%02d:%02d\n",
					val / 3600, (val / 60) % 60, val % 60);
			break;
		default:
			STR_PRINTF_RET(len, "<not implemented>\n");
			break;
	}

	return len;
}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
static int time_write(struct file *file, const char __user *buffer, unsigned long count, void *data)
#else
static ssize_t time_write(struct file *file, const char __user *buffer, size_t count, loff_t *off)
#endif
{
	char kbuf[2];

	if (count > 2)
		return count;
	if (copy_from_user(kbuf, buffer, count))
	{
		return -EFAULT;
	}
	if ((count == 2) && (kbuf[1] != '\n'))
		return count;
	if ((kbuf[0] < '0') || ('9' < kbuf[0]))
		return count;
	state = kbuf[0] - '0';
	return count;
}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
#else
static int time_open(struct inode *inode, struct file *file)
{
	return single_open(file, time_read, NULL);
}

static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.open = time_open,
	.read = seq_read,
	.write = time_write
};
#endif

static int __init proc_win_init(void)
{
	if ((parent = proc_mkdir("anil", NULL)) == NULL)
	{
		return -1;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
	if ((file = create_proc_entry("rel_time", 0666, parent)) == NULL)
#else
	if ((file = proc_create("rel_time", 0666, parent, &fops)) == NULL)
#endif
	{
		remove_proc_entry("anil", NULL);
		return -1;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
	file->read_proc = time_read;
	file->write_proc = time_write;
#endif
	if ((link = proc_symlink("rel_time_l", parent, "rel_time")) == NULL)
	{
		remove_proc_entry("rel_time", parent);
		remove_proc_entry("anil", NULL);
		return -1;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
	link->uid = 0;
	link->gid = 100;
#else
	proc_set_user(link, KUIDT_INIT(0), KGIDT_INIT(100));
#endif
	return 0;
}

static void __exit proc_win_exit(void)
{
	remove_proc_entry("rel_time_l", parent);
	remove_proc_entry("rel_time", parent);
	remove_proc_entry("anil", NULL);
}

module_init(proc_win_init);
module_exit(proc_win_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Kernel window /proc Demonstration Driver");
