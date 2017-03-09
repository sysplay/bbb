.global _asm_entry

.section RESET_VECTOR, "x"
_asm_entry:
	//LDR sp, =stack_top	/* Set stack pointer */
	//BL _clear_bss			/* Zero out the BSS */
	BL c_entry				/* C code entry point */
	//B .					/* Loop forever after return from C code */

/*
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
 */
