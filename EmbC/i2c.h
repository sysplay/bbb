#ifndef I2C_H
#define I2C_H

#include "common.h"

typedef enum
{
	standard,
	fast
} I2CMode;

void i2c_init(I2CMode mode);
void i2c_shut(void);
int i2c_master_tx(uint8_t addr, uint8_t *data, int len);
int i2c_master_rx(uint8_t addr, uint8_t *data, int len);
int i2c_master_tx_rx(uint8_t addr, uint8_t *tx_data, int tx_len, uint8_t *rx_data, int rx_len);

#endif
