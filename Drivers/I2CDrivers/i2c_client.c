#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/mod_devicetable.h>
#include <linux/log2.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

struct dummy_data {
	struct i2c_client *client;
	u8 *write_buff;
	u16 write_max;
	/* Character Driver Files */
	dev_t devt;
	struct cdev cdev;
	struct class *class;
};	

static ssize_t my_read(struct file* f, char *buf, size_t count, loff_t *f_pos)
{
	struct i2c_msg msg;
	char *tmp;
	struct dummy_data *dev = (struct dummy_data *)(f->private_data);
	struct i2c_adapter *adap = dev->client->adapter;
	int ret;

	if (count > 8192)
		count = 8192;

	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;

	msg.addr = 0x50; //client->addr;
	msg.flags = 0; //client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = tmp;
	printk("Invoking Transfer\n");
	ret = i2c_transfer(adap, &msg, 1); 
	if (ret >= 0)
		ret = copy_to_user(buf, tmp, count) ? -EFAULT : count;
	kfree(tmp);
	return ret;
}

static ssize_t my_write(struct file* f, const char *buf, size_t count, loff_t *f_pos)
{
	struct dummy_data *dev = (struct dummy_data *)(f->private_data);
	struct i2c_adapter *adap = dev->client->adapter;
	char *tmp;
	struct i2c_msg msg;
	int ret;

	tmp = memdup_user(buf, count);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);
	msg.addr = 0x50; //client->addr;
	msg.flags = 0; //client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = tmp;

	ret = i2c_transfer(adap, &msg, 1); 
	kfree(tmp);
	return (ret == 1 ? count : ret);
}

static int my_open(struct inode *i, struct file *f)
{
	//struct i2c_adapter *adap;
	struct dummy_data *dev = container_of(i->i_cdev, struct dummy_data, cdev);
	if (dev == NULL) {
		printk("Data is null\n");
		return -1;
	}
	f->private_data = dev;

	return 0;
}

static int my_close(struct inode *i, struct file *file)
{
	return 0;
}

struct file_operations fops = {
	.open = my_open,
	.release = my_close,
	.read = my_read,
	.write = my_write,
};

static int dummy_remove(struct i2c_client *client)
{
	struct dummy_data *dev;
	printk("Remove Invoked\n");
	dev = i2c_get_clientdata(client);
	cdev_del(&dev->cdev);
	device_destroy(dev->class, dev->devt);
	class_destroy(dev->class);
	unregister_chrdev_region(dev->devt, 1);
	return 0;

}
static int dummy_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct dummy_data *data;
	int init_result;

	printk("Probe Invoked\n");
	data = devm_kzalloc(&client->dev, sizeof(struct dummy_data), GFP_KERNEL);
	data->write_max = 32;
	data->write_buff = devm_kzalloc(&client->dev, data->write_max, GFP_KERNEL);
	i2c_set_clientdata(client, data);
	init_result = alloc_chrdev_region(&data->devt, 0, 1, "i2c_dmy");
	data->client = client;

	if( 0 > init_result )
	{
		printk(KERN_ALERT "Device Registration failed\n");
		unregister_chrdev_region(data->devt, 1);
		return -1;
	}
	printk("Major Nr: %d\n", MAJOR(data->devt));

	if ((data->class = class_create(THIS_MODULE, "i2cdummy")) == NULL)
	{
		printk( KERN_ALERT "Class creation failed\n" );
		unregister_chrdev_region(data->devt, 1);
		return -1;
	}
	if(device_create(data->class, NULL, data->devt, NULL, "i2c_dmy%d", 0) == NULL)
	{
		printk( KERN_ALERT "Device creation failed\n" );
		class_destroy(data->class);
		unregister_chrdev_region(data->devt, 1);
		return -1;
	}
	
	cdev_init(&data->cdev, &fops);

	if(cdev_add(&data->cdev, data->devt, 1) == -1)
	{
		printk( KERN_ALERT "Device addition failed\n" );
		device_destroy(data->class, data->devt);
		class_destroy(data->class);
		unregister_chrdev_region(data->devt, 1 );
		return -1;
	}
	return 0;
}

static const struct i2c_device_id dummy_ids[] = {
	{ "25c256", 0},
	{ "at25", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, dummy_ids);

static struct i2c_driver dummy_driver = {
	.driver = {
		.name = "at25",
		.owner = THIS_MODULE,
	},
	.probe = dummy_probe,
	.remove = dummy_remove,
	.id_table = dummy_ids,
};

static int __init dummy_init(void)
{
	return i2c_add_driver(&dummy_driver);
}
module_init(dummy_init);

static void __exit dummy_exit(void)
{
	i2c_del_driver(&dummy_driver);
}
module_exit(dummy_exit);

MODULE_DESCRIPTION("Dummy Client Driver");
MODULE_AUTHOR("Pradeep Tewani");
MODULE_LICENSE("GPL");

