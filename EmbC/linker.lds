ENTRY(_asm_entry) /* Entry point of the program */

SECTIONS
{
	. = 0x402F0400;			/* Internal SRAM start */
	.text :					/* Code placement */
	{
		startup.o (RESET_VECTOR)
		*(.text)
	}
	.data : { *(.data /*.rodata*/) }	/* Global and static variables initialized by the application code */
	.bss : { *(.bss) }		/* Global and static variables that are not initialized by the application code.
								The compiler initializes this data to 0. */
	stack_top = 0x4030B800;	/* 6KiB stack */
}
