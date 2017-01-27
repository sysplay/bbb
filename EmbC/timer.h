#ifndef TIMER_H

#define TIMER_H

#include "common.h"

//#define INTR_BASED_TIMER

void timer_init(uint32_t msecs);
void timer_shut(void);
#ifdef INTR_BASED_TIMER
void timer_handler_register(void (*handler)(void));
void timer_handler_unregister(void);
#endif

#endif
