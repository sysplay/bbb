.global _asm_entry

.section RESET_VECTOR, "x"
_asm_entry:
	LDR sp, =stack_top	/* Set stack pointer */
	BL c_entry			/* C code entry point */
	B .					/* Loop forever after return from C code */
