#ifndef ARM_H

#define ARM_H

typedef enum
{
	cm_usr = 0x10,
	cm_fiq = 0x11,
	cm_irq = 0x12,
	cm_svc = 0x13,
	cm_abt = 0x17,
	cm_und = 0x1B,
	cm_sys = 0x1F,
	cm_mask = 0x1F
} CPUMode;

static inline void mb(void)
{
	__asm__ __volatile__("dsb\n");
}
static inline void ib(void)
{
	__asm__ __volatile__("isb\n");
}
unsigned long cpu_mode_get(void);
void cpu_mode_set(CPUMode cm);
void neon_enable(void);
void neon_disable(void);

#endif
