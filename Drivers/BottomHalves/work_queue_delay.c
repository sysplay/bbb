#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

static struct workqueue_struct *my_wq;

typedef struct
{
	struct delayed_work d_work;
	int x;
} my_work_t;


my_work_t *work, *work2;

static void my_wq_function(struct work_struct *work)
{
	my_work_t *my_work = (my_work_t *)work;
	printk("my_work.x %d\n", my_work->x);
	kfree((void *)work);
	return;
}

int init_module(void)
{
	int ret;

	my_wq = create_workqueue("my_queue");
	if (my_wq)
	{
		work = kmalloc(sizeof(my_work_t), GFP_KERNEL);
		if (work)
		{
			INIT_DELAYED_WORK((struct delayed_work *)work, my_wq_function);
			work->x = 1;
			ret = queue_delayed_work(my_wq, (struct delayed_work *)work, 400);
		}
		work2 = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
		if (work2)
		{
			INIT_DELAYED_WORK((struct delayed_work *)work2, my_wq_function);
			work2->x = 2;
			ret = queue_delayed_work(my_wq, (struct delayed_work *)work2, 200);
		}
	}
	return 0;
}

void cleanup_module(void)
{
	flush_workqueue(my_wq);
	destroy_workqueue(my_wq);
	return;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Delayed Work Queues Demo");
