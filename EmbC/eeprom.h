#ifndef EEPROM_H

#define EEPROM_H

#include "common.h"

void eeprom_init(void);
void eeprom_shut(void);
int eeprom_write(uint8_t *addr, uint8_t *data, int len);
int eeprom_read(uint8_t *addr, uint8_t *data, int len);

#endif
