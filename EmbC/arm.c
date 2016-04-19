#include "arm.h"

unsigned long cpu_mode_get(void)
{
	unsigned long r;

	__asm__ __volatile__("mrs %0, cpsr\n" : "=r" (r) : : );
	return (r & cm_mask);
}

void cpu_mode_set(CPUMode cm)
{
	unsigned long r = 0xC0 | cm; // IRQ & FIQ disabled

	__asm__ __volatile__("msr cpsr, %0\n" : "=r" (r) : : );
}

/* The following code needs -mfpu=neon to be added as compiler flag */

void neon_enable(void)
{
	__asm__ __volatile__("mrc p15, 0, r1, c1, c0, 2\n"); // r1 = Access Control Register
	__asm__ __volatile__("orr r1, r1, (0xF << 20)\n"); // Enable full access for p10,11
	__asm__ __volatile__("mcr p15, 0, r1, c1, c0, 2\n"); // Access Control Register = r1
	__asm__ __volatile__("mov r0, 0x40000000\n");
	__asm__ __volatile__("vmsr fpexc, r0\n"); // Set Neon/VFP Enable bit - fmxr <=> vmsr
}

void neon_disable(void)
{
	__asm__ __volatile__("mov r0, 0x00000000\n");
	__asm__ __volatile__("vmsr fpexc, r0\n"); // Clear Neon/VFP Enable bit - fmxr <=> vmsr
	__asm__ __volatile__("mrc p15, 0, r1, c1, c0, 2\n"); // r1 = Access Control Register
	__asm__ __volatile__("bic r1, r1, (0xF << 20)\n"); // Disable full access for p10,11
	__asm__ __volatile__("mcr p15, 0, r1, c1, c0, 2\n"); // Access Control Register = r1
}
