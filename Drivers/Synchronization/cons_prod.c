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
#include <linux/random.h>
#include <linux/slab.h>

static struct task_struct *thread_cons[2], *thread_prod;
static int thread_id[2];

struct job 
{
	int Mydata;
	struct job* next;
};

static struct job* job_queue;

DEFINE_MUTEX(my_mutex);

static void process_job(void *thread_id, struct job* next_job)
{
	unsigned int i, id;
	if (thread_id == NULL) 
	{
		printk("Invalid Thread id\n");
		return;
	}
	id = *(int *)thread_id;
	printk("\nThe data from the thread %u is %d\n",id, next_job->Mydata);
	get_random_bytes(&i, sizeof(i));
	ssleep(i % 5);
}

void enqueue_job (struct job* new_job)
{
	static int i = 0;

	mutex_lock (&my_mutex);
	new_job->next = job_queue;
	job_queue = new_job;
	job_queue->Mydata=i++;
	mutex_unlock (&my_mutex);
	printk("\n Job no %d is added.\n",i-1);
}

int producer(void* arg)
{
	struct job *new_job;
	unsigned int i;

	allow_signal(SIGKILL);

	while (!kthread_should_stop())
	{
		new_job=(struct job*)kmalloc(sizeof(struct job), GFP_KERNEL);
		if (new_job == NULL) 
		{
			printk("Unable to allocate the job structure\n");
			return -1;
		}

		enqueue_job(new_job);
		get_random_bytes(&i, sizeof(i));
		ssleep(i % 5);
		if (signal_pending(current))
			break;
	}
	printk(KERN_INFO "Producer Thread Stopping\n");
	thread_prod = NULL;
	do_exit(0);
}

static int thread_fn(void *thread_id)
{
	int id;

	allow_signal(SIGKILL);

	while (!kthread_should_stop()) 
	{
		struct job* next_job;
		mutex_lock(&my_mutex);
		if (job_queue == NULL)
			next_job = NULL;
		else
		{
			next_job = job_queue;
			job_queue = job_queue->next;
		}
		mutex_unlock (&my_mutex);
		if (next_job == NULL)
			break;
		process_job (thread_id, next_job);
		kfree (next_job);
		if (signal_pending(current))
			break;
	}
	id = *(int *)thread_id;
	thread_cons[id] = NULL;
	printk(KERN_INFO "Consumer Thread Stopping\n");
	do_exit(0);
}


static int __init init_consprod(void)
{
	int i;
	char buff[20];
	struct job *new_job;

	for (i = 0; i < 6; i++) 
	{
		new_job=(struct job*)kmalloc(sizeof(struct job), GFP_KERNEL);
		if (new_job == NULL) {
			printk("Failed to allocate the memory for job\n");
			return -1;
		}
		enqueue_job(new_job);
	}

	for (i = 0; i < 2; i++) 
	{
		thread_id[i] = i;
		sprintf(buff, "thread_con%d", i);
		thread_cons[i] = kthread_run(thread_fn, &thread_id[i], buff);
		if (thread_cons[i])
			printk(KERN_INFO "Thread %d created\n", i);
		else 
		{
			printk(KERN_INFO "Thread creation failed\n");
			return -1;
		}
	}
	thread_prod = kthread_run(producer, NULL, "myprod");
	if (thread_prod)
		printk(KERN_INFO "Producer Thread created\n");
	else 
	{
		printk(KERN_INFO "Producer thread creation failed\n");
		return -1;
	}

	return 0;
}

static void __exit cleanup_consprod(void)
{
	int i;
	printk("Cleaning Up\n");
	for (i = 0; i < 2; i++)
	{
		if (thread_cons[i] != NULL)
		{
			kthread_stop(thread_cons[i]);
			printk(KERN_INFO "Thread %d stopped", i);
		}
	}
	if (thread_prod != NULL)
	{
		kthread_stop(thread_prod);
		printk(KERN_INFO "Thread prod stopped");
	}
}

module_init(init_consprod);
module_exit(cleanup_consprod);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Consumer Producer Demo");
