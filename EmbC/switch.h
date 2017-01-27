#ifndef SWITCH_H

#define SWITCH_H

#include "common.h"

#define INTR_BASED_SWITCH

enum
{
	sw_released,
	sw_pressed
};

void switch_init(void);
void switch_shut(void);
uint32_t switch_pressed(void);
uint32_t switch_read(void);
#ifdef INTR_BASED_SWITCH
void switch_handler_register(void (*handler)(void));
void switch_handler_unregister(void);
#endif

#endif
