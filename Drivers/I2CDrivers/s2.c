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
	u16 status, cnt = 3;
	u16 idx = 0, i;
	u8 tx_buf[6] = {0X00, 0X50, 0x95};
	int i2c_error = 0;
	int k = 7;
	w &= ~(0x3f);
	w |= OMAP_I2C_BUF_TXFIF_CLR;
	omap_i2c_write_reg(dev, OMAP_I2C_BUF_REG, w);
	omap_i2c_write_reg(dev, OMAP_I2C_SA_REG, 0x50); /* Slave Address */
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
		printk("Status = %x\n", status);
		if (status == 0) {
			i2c_error = -ETIMEDOUT;
			goto wr_exit;
		}
		if (status & OMAP_I2C_STAT_XRDY) {
			printk("Got XRDY\n");
			omap_i2c_write_reg(dev, OMAP_I2C_DATA_REG, tx_buf[idx++]);
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_XRDY);   
			continue;   
		}
		if (status & OMAP_I2C_STAT_ARDY) {	
			printk("Got ARDY\n");
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_ARDY);   
			break;
		}
	}
	if (k <= 0) {
		printk("TX Timed out\n");
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
	
	return omap_i2c_read_msg(dev, msg, 1); /* For varifying i2c_writing */

}

