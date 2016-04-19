ENTRY(_asm_entry) /* Entry point of the program */

SECTIONS
{
	. = 0x402F0400;			/* Internal SRAM start */
	.text :					/* Code placement */
	{
		startup_first.o (RESET_VECTOR)
		*(.text)
	}
	.data : { *(.data) }	/* Global and static variables initialized by the application code */
	.bss : { *(.bss) }		/* Global and static variables that are not initialized by the application code.
								The compiler initializes this data to 0. */
	. = ALIGN(8);
	. = . + 0x1000;			/* 4KiB of stack memory */
	stack_top = .;			/* stack grows upwards */
}

