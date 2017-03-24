.global _asm_entry
.global _undefined_instruction
.global _software_interrupt
.global _prefetch_abort
.global _data_abort
.global _interrupt
.global _fast_interrupt

.set MODE_IRQ, 0x12
.set MODE_SVC, 0x13
.set I_F_BIT, 0xC0

.set SVC_STACK_SIZE, 0x1000 /* 4 KiB */

.section RESET_VECTOR, "x"
_asm_entry:
	LDR r0, =stack_top
	MOV sp, r0			/* Set supervisor mode stack pointer */
	SUB r0, r0, #SVC_STACK_SIZE
	MSR cpsr, #MODE_IRQ | I_F_BIT
	MOV sp, r0			/* Set irq mode stack pointer */
	MSR cpsr, #MODE_SVC | I_F_BIT
	BL _clear_bss		/* Zero out the BSS */
	BL c_entry			/* C code entry point */
	B .					/* Loop forever after return from C code */

_clear_bss:
	MOV r0, #0
	LDR r1, =bss_begin
	LDR r2, =bss_end
1:
	CMP r1, r2
	BEQ 2f
	STR r0, [r1]
	ADD r1, #4
	B 1b
2:
	BX lr

.align	2
_undefined_instruction:
	B undefined_instruction_vector	/* Undefined Instruction */
.align	2
_software_interrupt:
	B software_interrupt_vector		/* Software Interrupt (SWI) */
.align	2
_prefetch_abort:
	B prefetch_abort_vector			/* Prefetch Abort */
.align	2
_data_abort:
	B data_abort_vector				/* Data Abort */
.align	2
_interrupt:
	B interrupt_vector				/* IRQ (Interrupt) */
.align	2
_fast_interrupt:
	B fast_interrupt_vector			/* FIQ (Fast Interrupt) */
