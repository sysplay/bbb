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

	return 0;

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

