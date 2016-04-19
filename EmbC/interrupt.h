#ifndef INTERRUPT_H

#define INTERRUPT_H

#include "common.h"

#define GPIO2A_IRQ 32
#define GPIO2B_IRQ 33
#define GPIO3A_IRQ 62
#define GPIO3B_IRQ 63
#define TIMER0_IRQ 66
#define I2C0_IRQ 70
#define UART0_IRQ 72
#define GPIO0A_IRQ 96
#define GPIO0B_IRQ 97
#define GPIO1A_IRQ 98
#define GPIO1B_IRQ 99

unsigned long interrupt_vectors_address_get(void);
void interrupt_vectors_address_set(unsigned long v);

void interrupt_init(void);
void interrupt_shut(void);
void interrupt_handler_register(uint8_t irq, void (*handler)(void));
void interrupt_handler_unregister(uint8_t irq);
void interrupt_enable(uint8_t irq);
void interrupt_disable(uint8_t irq);

#endif
