/* ENTRY(_asm_entry) */

SECTIONS
{
	. = 0x402F0400;
	.text :
	{
		/* first_startup.o(RESET_VECTOR) */
		*(.text)
	}
	.data : { *(.data) }
	.bss :
	{
		/* bss_begin = .; */
		*(.bss)
		/*
		. = ALIGN(4);
		bss_end = .;
		 */
	}
	/*
	. = ALIGN(8);
	. = . + 0x1000;
	stack_top = .;
	 */
}
