#include "i2c_char.h"

/*
 * Low level master read/write transaction.
 */
int i2c_xfer_msg(struct omap_i2c_dev *dev,
			     struct i2c_msg *msg, int stop)
{
	unsigned long timeout;
	u16 w, i;

	printk("###addr: 0x%04x, len: %d, flags: 0x%x, stop: %d ###\n",
		msg->addr, msg->len, msg->flags, stop);

	if (msg->len == 0)
		return -EINVAL;

	dev->receiver = !!(msg->flags & I2C_M_RD);
	omap_i2c_resize_fifo(dev, msg->len, dev->receiver);
	
	if (!dev->receiver) {
		printk("#### Sending %d bytes #####\n", msg->len);
		for (i = 0; i < msg->len; i++) {
			if (!(i % 15))
				printk("\n");
			printk("%x\t", msg->buf[i]);
		}
	}
	printk("\n");

	omap_i2c_write_reg(dev, OMAP_I2C_SA_REG, msg->addr);

	dev->buf = msg->buf;
	dev->buf_len = msg->len;

	/* make sure writes to dev->buf_len are ordered */
	barrier();

	omap_i2c_write_reg(dev, OMAP_I2C_CNT_REG, dev->buf_len);

	/* Clear the FIFO Buffers */
	w = omap_i2c_read_reg(dev, OMAP_I2C_BUF_REG);
	w |= OMAP_I2C_BUF_RXFIF_CLR | OMAP_I2C_BUF_TXFIF_CLR;
	omap_i2c_write_reg(dev, OMAP_I2C_BUF_REG, w);
	w = omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG);

	INIT_COMPLETION(dev->cmd_complete);
	dev->cmd_err = 0;

	w = OMAP_I2C_CON_EN | OMAP_I2C_CON_MST | OMAP_I2C_CON_STT | OMAP_I2C_CON_STP;

	if (!(msg->flags & I2C_M_RD))
		w |= OMAP_I2C_CON_TRX;
	printk("#### Initiatiate the I2C transaction ####\n");
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, w);

	/*
	 * REVISIT: We should abort the transfer on signals, but the bus goes
	 * into arbitration and we're currently unable to recover from it.
	 */
	timeout = wait_for_completion_timeout(&dev->cmd_complete,
						OMAP_I2C_TIMEOUT);
	if (timeout == 0) {
		dev_err(dev->dev, "controller timed out\n");
		printk("Controller Timed Out\n");
		omap_i2c_reset(dev);
		__omap_i2c_init(dev);
		return -ETIMEDOUT;
	}
	/* Returns 1 if command was transferred without any error */
	if (likely(!dev->cmd_err))
		return 1;

	/* We have an error */
	if (dev->cmd_err & (OMAP_I2C_STAT_AL | OMAP_I2C_STAT_ROVR |
			    OMAP_I2C_STAT_XUDF)) {
		printk("#### ERROR in Transfer ####\n");
		omap_i2c_reset(dev);
		__omap_i2c_init(dev);
		return -EIO;
	}

	if (dev->cmd_err & OMAP_I2C_STAT_NACK) {
		if (msg->flags & I2C_M_IGNORE_NAK)
			return 1;
		printk("#### NAK from the receiver ####\n");

		w = omap_i2c_read_reg(dev, OMAP_I2C_CON_REG);
		w |= OMAP_I2C_CON_STP;
		omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, w);
		return -EREMOTEIO;
	}
	return -EIO;
}
