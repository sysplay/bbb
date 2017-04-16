#include "i2c_char.h"

int i2c_write(struct omap_i2c_dev *dev, struct i2c_msg *msg, size_t count)
{
	//Set the TX FIFO Threshold and clear the FIFO's
	//Set the slave address
	//update the count register
	//update the CON Register to start the transaction with master mode, transmitter
	//Wait for anything interesting to happen on the bus
	//Check for the status - XRDY, then write the data in data register
	//Check if ARDY is come
	
	/* Set the threshold to 0 and clear buffers */
	u16 w = omap_i2c_read_reg(dev, OMAP_I2C_BUF_REG);
	u16 status, cnt = msg->len;
	u16 idx = 0, i;
	//u8 tx_buf[6] = {0X00, 0X50, 0x94};
	u8 *tx_buf = msg->buf;
	int i2c_error = 0;
	//int k = 7;
	int k = msg->len + 4;
	w &= ~(0x3f);
	w |= OMAP_I2C_BUF_TXFIF_CLR;
	omap_i2c_write_reg(dev, OMAP_I2C_BUF_REG, w);
	omap_i2c_write_reg(dev, OMAP_I2C_SA_REG, msg->addr); /* Slave Address */
	omap_i2c_write_reg(dev, OMAP_I2C_CNT_REG, cnt); /* Count of 1*/
	printk("##### Sending %d bytes on the I2C bus ####\n", cnt);
	printk("#### Tx buff ####\n");
	for (i = 0; i < cnt; i++) 
		printk("%x\t", tx_buf[i]);
	printk("\n");
	w = (OMAP_I2C_CON_MST | OMAP_I2C_CON_STT | OMAP_I2C_CON_EN | OMAP_I2C_CON_STP
			| OMAP_I2C_CON_TRX);
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, w); /* Control Register */

	while (k--) {
		status = wait_for_event(dev);
		printk("#### Status = %x #####\n", status);
		if (status == 0) {
			i2c_error = -ETIMEDOUT;
			goto wr_exit;
		}
		if (status & OMAP_I2C_STAT_XRDY) {
			printk("#### Got XRDY ####\n");
			omap_i2c_write_reg(dev, OMAP_I2C_DATA_REG, tx_buf[idx++]);
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_XRDY);   
			continue;   
		}
		if (status & OMAP_I2C_STAT_ARDY) {	
			printk("#### Got ARDY ####\n");
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_ARDY);   
			break;
		}
	}
	if (k <= 0) {
		printk("#### TX Timed out ####\n");
		i2c_error = -ETIMEDOUT;
	}
wr_exit:
	flush_fifo(dev);
	omap_i2c_write_reg(dev, OMAP_I2C_STAT_REG, 0XFFFF); /* Slave Address */
	return i2c_error;
}

int i2c_read(struct omap_i2c_dev *dev, struct i2c_msg *msg, size_t len)
{	
	//Set the RX FIFO Threshold and clear the FIFO's
	//Set the slave address
	//update the count register
	//update the CON Register to start the transaction with master mode, Reciever
	//Wait for anything interesting to happen on the bus
	//Check for the status - RRDY, then write the data in data register
	//Check if ARDY is come
	
	u16 w = omap_i2c_read_reg(dev, OMAP_I2C_BUF_REG);
	u16 status;
	u16 idx = 0, i, cnt = msg->len;
	u8 *rx_buf = msg->buf;
	//u8 rx_buf[6];
	int i2c_error = 0;
	//int k = 7;
	int k = msg->len + 4;
	w &= ~(0x3f << 8);
	w |= OMAP_I2C_BUF_RXFIF_CLR;
	omap_i2c_write_reg(dev, OMAP_I2C_BUF_REG, w);
	omap_i2c_write_reg(dev, OMAP_I2C_SA_REG, msg->addr); /* Slave Address */
	omap_i2c_write_reg(dev, OMAP_I2C_CNT_REG, cnt); /* Count of 1*/
	printk("##### Receiving %d bytes from the I2C bus ####\n", cnt);
	w = (OMAP_I2C_CON_MST | OMAP_I2C_CON_STT | OMAP_I2C_CON_EN | OMAP_I2C_CON_STP);
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, w); /* Control Register */

	while (k--) {
		status = wait_for_event(dev);
		printk("### Status = %x ###\n", status);
		if (status == 0) {
			i2c_error = -ETIMEDOUT;
			goto wr_exit;
		}
		if (status & OMAP_I2C_STAT_RRDY) {
			printk("#### Got RRDY ####\n");
			rx_buf[idx] = omap_i2c_read_reg(dev, OMAP_I2C_DATA_REG);
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_RRDY);   
			printk("rx[%d] = %x\n", idx, rx_buf[idx]);
			idx++;
			continue;   
		}
		if (status & OMAP_I2C_STAT_ARDY) {	
			printk("#### Got ARDY ####\n");
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_ARDY);   
			break;
		}
	}
	if (k <= 0) {
		printk("### RX Timed out ###\n");
		i2c_error = -ETIMEDOUT;
	} else {
		printk("#### Rx buff ####\n");
		for (i = 0; i < cnt; i++) {
		       if (!(i % 15)) 
			       printk("\n");
			printk("%x\t", rx_buf[i]);
		}
		printk("\n");
	}
wr_exit:
	flush_fifo(dev);
	omap_i2c_write_reg(dev, OMAP_I2C_STAT_REG, 0XFFFF); /* Slave Address */
	return i2c_error;

	return 0;
}

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
