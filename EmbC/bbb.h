#ifndef BBB_H

#define BBB_H

#include "common.h"

#define _REG_ADDR(base, off) ((volatile uint32_t *)((base) + (off)))
#define _REG(base, off) (*_REG_ADDR(base, off))

// Clock Registers - Sec 8.1.12
// NB Module clocks are required even to write into the registers of the corresponding modules

// For working of peripherals - no control for UART0, GPIO0, TIMER0, I2C0
#define _CM_PER_REG(off) _REG(0x44E00000, off)
#define CM_PER_GPIO1_CLKCTRL _CM_PER_REG(0xAC)
#define CM_PER_GPIO2_CLKCTRL _CM_PER_REG(0xB0)
#define CM_PER_GPIO3_CLKCTRL _CM_PER_REG(0xB4)

// For wake up modules of peripherals - no control for GPIO1, GPIO2, GPIO3
#define _CM_WKUP_REG(off) _REG(0x44E00400, off)
#define CM_WKUP_GPIO0_CLKCTRL _CM_WKUP_REG(0x08)
#define CM_WKUP_TIMER0_CLKCTRL _CM_WKUP_REG(0x10)
#define CM_WKUP_UART0_CLKCTRL _CM_WKUP_REG(0xB4)
#define CM_WKUP_I2C0_CLKCTRL _CM_WKUP_REG(0xB8)

// For controlling I/O multiplexing
#define _CTRL_MOD_REG(off) _REG(0x44E10000, off)
#define CTRL_MOD_CONF_I2C0_SDA _CTRL_MOD_REG(0x988)
#define CTRL_MOD_CONF_I2C0_SCL _CTRL_MOD_REG(0x98C)

// Functional Registers

#define _TIMER0_REG(off) _REG(0x44E05000, off)
#define TIMER0_IRQSTATUS_RAW _TIMER0_REG(0x24)
#define TIMER0_IRQSTATUS _TIMER0_REG(0x28)
#define TIMER0_IRQENABLE_SET _TIMER0_REG(0x2C)
#define TIMER0_IRQENABLE_CLR _TIMER0_REG(0x30)
#define TIMER0_TCLR _TIMER0_REG(0x38)
#define TIMER0_TCRR _TIMER0_REG(0x3C)
#define TIMER0_TLDR _TIMER0_REG(0x40)
#define TIMER0_TTGR _TIMER0_REG(0x44)

#define _UART0_REG(off) _REG(0x44E09000, off)
#define UART0_THR _UART0_REG(0x0)
#define UART0_RHR _UART0_REG(0x0)
#define UART0_DLL _UART0_REG(0x0)
#define UART0_IER _UART0_REG(0x4)
#define UART0_DLH _UART0_REG(0x4)
#define UART0_IIR _UART0_REG(0x8)
#define UART0_LCR _UART0_REG(0xC)
#define UART0_MCR _UART0_REG(0x10)
#define UART0_LSR _UART0_REG(0x14)
#define UART0_MDR1 _UART0_REG(0x20)

#define _I2C0_REG(off) _REG(0x44E0B000, off)
#define I2C0_IRQSTATUS_RAW _I2C0_REG(0x24) // Raw intr status - to use w/o enabling
#define I2C0_IRQSTATUS _I2C0_REG(0x28) // Intr status
#define I2C0_IRQENABLE_SET _I2C0_REG(0x2C) // Intr mask set
#define I2C0_IRQENABLE_CLR _I2C0_REG(0x30) // Intr mask clear
#define I2C0_CNT _I2C0_REG(0x98)
#define I2C0_DATA _I2C0_REG(0x9C)
#define I2C0_CON _I2C0_REG(0xA4)
#define I2C0_OA _I2C0_REG(0xA8)
#define I2C0_SA _I2C0_REG(0xAC)
#define I2C0_PSC _I2C0_REG(0xB0)
#define I2C0_SCLL _I2C0_REG(0xB4)
#define I2C0_SCLH _I2C0_REG(0xB8)

#define _GPIO1_REG(off) _REG(0x4804C000, off)
#define GPIO1_IRQSTATUS_RAW_0 _GPIO1_REG(0x24) // Raw intr status - to use w/o enabling
#define GPIO1_IRQSTATUS_RAW_1 _GPIO1_REG(0x28)
#define GPIO1_IRQSTATUS_0 _GPIO1_REG(0x2C) // Intr status
#define GPIO1_IRQSTATUS_1 _GPIO1_REG(0x30)
#define GPIO1_IRQSTATUS_SET_0 _GPIO1_REG(0x34) // Intr mask set
#define GPIO1_IRQSTATUS_SET_1 _GPIO1_REG(0x38)
#define GPIO1_IRQSTATUS_CLR_0 _GPIO1_REG(0x3C) // Intr mask clear
#define GPIO1_IRQSTATUS_CLR_1 _GPIO1_REG(0x40)
#define GPIO1_OE _GPIO1_REG(0x134)
#define GPIO1_DATAIN _GPIO1_REG(0x138)
#define GPIO1_DATAOUT _GPIO1_REG(0x13C)
#define GPIO1_LEVELDETECT0 _GPIO1_REG(0x140)
#define GPIO1_LEVELDETECT1 _GPIO1_REG(0x144)
#define GPIO1_RISINGDETECT _GPIO1_REG(0x148)
#define GPIO1_FALLINGDETECT _GPIO1_REG(0x14C)
#define GPIO1_DEBOUNCENABLE _GPIO1_REG(0x150)
#define GPIO1_DEBOUNCINGTIME _GPIO1_REG(0x154)
#define GPIO1_CLEARDATAOUT _GPIO1_REG(0x190)
#define GPIO1_SETDATAOUT _GPIO1_REG(0x194)

#define _GPIO2_REG(off) _REG(0x481AC000, off)
#define GPIO2_IRQSTATUS_RAW_0 _GPIO2_REG(0x24) // Raw intr status - to use w/o enabling
#define GPIO2_IRQSTATUS_RAW_1 _GPIO2_REG(0x28)
#define GPIO2_IRQSTATUS_0 _GPIO2_REG(0x2C) // Intr status
#define GPIO2_IRQSTATUS_1 _GPIO2_REG(0x30)
#define GPIO2_IRQSTATUS_SET_0 _GPIO2_REG(0x34) // Intr mask set
#define GPIO2_IRQSTATUS_SET_1 _GPIO2_REG(0x38)
#define GPIO2_IRQSTATUS_CLR_0 _GPIO2_REG(0x3C) // Intr mask clear
#define GPIO2_IRQSTATUS_CLR_1 _GPIO2_REG(0x40)
#define GPIO2_OE _GPIO2_REG(0x134)
#define GPIO2_DATAIN _GPIO2_REG(0x138)
#define GPIO2_DATAOUT _GPIO2_REG(0x13C)
#define GPIO2_LEVELDETECT0 _GPIO2_REG(0x140)
#define GPIO2_LEVELDETECT1 _GPIO2_REG(0x144)
#define GPIO2_RISINGDETECT _GPIO2_REG(0x148)
#define GPIO2_FALLINGDETECT _GPIO2_REG(0x14C)
#define GPIO2_DEBOUNCENABLE _GPIO2_REG(0x150)
#define GPIO2_DEBOUNCINGTIME _GPIO2_REG(0x154)
#define GPIO2_CLEARDATAOUT _GPIO2_REG(0x190)
#define GPIO2_SETDATAOUT _GPIO2_REG(0x194)

#define _GPIO3_REG(off) _REG(0x481AE000, off)

#define _INTC_REG(off) _REG(0x48200000, off)
#define INTC_SIR_IRQ _INTC_REG(0x40) // Current IRQ number
#define INTC_SIR_FIQ _INTC_REG(0x44) // Current FIQ number
#define INTC_CONTROL _INTC_REG(0x48)
#define INTC_IRQ_PRIORITY _INTC_REG(0x60) // Current IRQ priority
#define INTC_FIQ_PRIORITY _INTC_REG(0x64) // Current FIQ priority
#define INTC_THRESHOLD _INTC_REG(0x68)
#define INTC_ITR0 _INTC_REG(0x80) // Raw Intr status: 0-31
#define INTC_MIR0 _INTC_REG(0x84) // Intr mask: 0-31
#define INTC_MIR_CLEAR0 _INTC_REG(0x88) // Intr mask clear: 0-31
#define INTC_MIR_SET0 _INTC_REG(0x8C) // Intr mask set: 0-31
#define INTC_PENDING_IRQ0 _INTC_REG(0x98) // IRQ Intr status: 0-31
#define INTC_PENDING_FIQ0 _INTC_REG(0x9C) // FIQ Intr status: 0-31
#define INTC_ITR1 _INTC_REG(0xA0) // Raw Intr status: 32-63
#define INTC_MIR1 _INTC_REG(0xA4) // Intr mask: 32-63
#define INTC_MIR_CLEAR1 _INTC_REG(0xA8) // Intr mask clear: 32-63
#define INTC_MIR_SET1 _INTC_REG(0xAC) // Intr mask set: 32-63
#define INTC_PENDING_IRQ1 _INTC_REG(0xB8) // IRQ Intr status: 32-63
#define INTC_PENDING_FIQ1 _INTC_REG(0xBC) // FIQ Intr status: 32-63
#define INTC_ITR2 _INTC_REG(0xC0) // Raw Intr status: 64-95
#define INTC_MIR2 _INTC_REG(0xC4) // Intr mask: 64-95
#define INTC_MIR_CLEAR2 _INTC_REG(0xC8) // Intr mask clear: 64-95
#define INTC_MIR_SET2 _INTC_REG(0xCC) // Intr mask set: 64-95
#define INTC_PENDING_IRQ2 _INTC_REG(0xD8) // IRQ Intr status: 64-95
#define INTC_PENDING_FIQ2 _INTC_REG(0xDC) // FIQ Intr status: 64-95
#define INTC_ITR3 _INTC_REG(0xE0) // Raw Intr status: 96-127
#define INTC_MIR3 _INTC_REG(0xE4) // Intr mask: 96-127
#define INTC_MIR_CLEAR3 _INTC_REG(0xE8) // Intr mask clear: 96-127
#define INTC_MIR_SET3 _INTC_REG(0xEC) // Intr mask set: 96-127
#define INTC_PENDING_IRQ3 _INTC_REG(0xF8) // IRQ Intr status: 96-127
#define INTC_PENDING_FIQ3 _INTC_REG(0xFC) // FIQ Intr status: 96-127
#define INTC_ILR(irq) _INTC_REG(0x100 + 4 * irq) // Intr priority & IRQ/FIQ steering

/*
// Processor Core Registers - Programmable Real-Time Unit & Industrial Communication Subsystem (PRU-ICSS)

#define _PRUICSS_REG(off) _REG(0x4A300000, off)
#define _PRUICSS_INTC_REG(off) _PRUICSS_REG(0x20000 + off)
#define INTC_GER _PRUICSS_INTC_REG(0x10)
 */

#endif
