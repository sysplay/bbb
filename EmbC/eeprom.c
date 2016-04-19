/* 24c256 */
#include "i2c.h"
#include "common.h"
#include "eeprom.h"

#define EEP_DEV_ADDR 0x50

void eeprom_init(void)
{
	i2c_init(standard);
}
void eeprom_shut(void)
{
	i2c_shut();
}

int eeprom_write(uint8_t *addr, uint8_t *data, int len)
{
	unsigned long eeprom_addr = (unsigned long)(addr);
	uint8_t eeprom_data[10];
	int i, cnt, ret;

	for (i = 0; i < (len >> 3); i++)
	{
		eeprom_data[0] = (eeprom_addr >> 8) & 0xFF;
		eeprom_data[1] = eeprom_addr & 0xFF;
		for (cnt = 0; cnt < 8; cnt++)
			eeprom_data[2 + cnt] = data[(i << 3) + cnt];
		if ((ret = i2c_master_tx(EEP_DEV_ADDR, eeprom_data, 10)) != 0)
			return ret;
		eeprom_addr += 8;
	}
	if (len & 0x7)
	{
		eeprom_data[0] = (eeprom_addr >> 8) & 0xFF;
		eeprom_data[1] = eeprom_addr & 0xFF;
		for (cnt = 0; cnt < (len & 0x7); cnt++)
			eeprom_data[2 + cnt] = data[(len & ~0x7) + cnt];
		if ((ret = i2c_master_tx(EEP_DEV_ADDR, eeprom_data, 2 + (len & 0x7))) != 0)
			return ret;
	}
	return 0;
}

int eeprom_read(uint8_t *addr, uint8_t *data, int len)
{
	unsigned long eeprom_addr = (unsigned long)(addr);
	uint8_t eeprom_data[2] = { ((eeprom_addr >> 8) & 0xFF), (uint8_t)(eeprom_addr & 0xFF)};

	return i2c_master_tx_rx(EEP_DEV_ADDR, eeprom_data, 2, data, len);
}
