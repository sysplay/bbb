#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>  // for threads
#include <linux/sched.h>  // for task_struct
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>


static struct task_struct *thread1,*thread2;
static spinlock_t my_lock;
static volatile int i = 0;

static u8 gpio_sw = 72;
static int irq_line;

static irqreturn_t button_irq_handler(int irq, void *data)
{
	printk("GPIO value is %d\n", gpio_get_value(gpio_sw));
	i = 1;
	return IRQ_HANDLED;
}

int thread_fn1(void *dummy) 
{
	int j;
	allow_signal(SIGKILL);
	for (j = 0; j < 2; j++) 
	{
		printk("Entering in a Lock\n");
		spin_lock(&my_lock);
		while (i == 0);
		spin_unlock(&my_lock);
		printk("Out of spin lock\n");
	}
	thread1 = NULL;
	do_exit(0);
}

int thread_fn2(void *dummy) 
{
	allow_signal(SIGKILL);
	printk("Thread 2 going to sleep\n");
	ssleep(5);
	i = 1;
	printk("Thread 2 out of sleep\n");
	thread2 = NULL;
	return 0;
}

int spinlock_init (void) 
{
	int irq_req_res;
	spin_lock_init(&my_lock);
	gpio_request_one(gpio_sw, GPIOF_IN, "button");
	if ( (irq_line = gpio_to_irq(gpio_sw)) < 0)
	{
		printk(KERN_ALERT "Gpio %d cannot be used as interrupt",gpio_sw);
		return -EINVAL;
	}

	if ( (irq_req_res = request_irq(irq_line, button_irq_handler, 
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "gpio_change_state", NULL)) < 0) 
	{
		printk(KERN_ERR "Keypad: registering irq failed\n");
		return -EINVAL;
	}
	thread1 = kthread_create(thread_fn1, NULL, "thread1");
	if(thread1)
	{
		wake_up_process(thread1);
	}
	thread2 = kthread_create(thread_fn2, NULL, "thread2");

	if(thread2)
	{
		wake_up_process(thread2);
	}
	
	return 0;
}


void spinlock_cleanup(void) 
{
	free_irq(irq_line, NULL);
	if (thread1 != NULL) 
	{
		kthread_stop(thread2);
		printk(KERN_INFO "Thread 1 stopped");
	}
	if (thread2 != NULL) 
	{
		kthread_stop(thread2);
		printk(KERN_INFO "Thread 2 stopped");
	}

}
module_init(spinlock_init);
module_exit(spinlock_cleanup);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("SysPlay Workshops <workshop@sysplay.in>");
MODULE_DESCRIPTION("Spin Lock Demo");
