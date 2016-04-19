#ifndef LEDS_H

#define LEDS_H

#include "common.h"

void leds_init(void);
void leds_shut(void);
void leds_on(uint32_t led_no);
void leds_off(uint32_t led_no);
void leds_toggle(uint32_t led_no);

#endif
