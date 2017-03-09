ENTRY(_asm_entry) /* Entry point of the program */

SECTIONS
{
	. = 0x402F0400; /* Internal SRAM start */
	.text : /* Code placement */
	{
		startup.o (RESET_VECTOR)
		*(.text)
	}
	.data : { *(.data /*.rodata*/) } /* Global and static variables initialized by the application code */
	.bss : /* Global and static variables that are not initialized by the application code */
	{
		bss_begin = .;
		*(.bss)
		. = ALIGN(4);
		bss_end = .;
	}
	stack_top = 0x4030B800;	/* 6KiB stack */
}
