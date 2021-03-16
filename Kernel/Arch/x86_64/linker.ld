ENTRY(_start)

SECTIONS
{
	/* This kernel will be at -2GiB in the 64-bit virtual address space */
	/* It gets placed there by Kickstart, therefore we can just link directly here */
	. = 0xFFFFFFFF80000000;

	/* Put the kernel sections like they should be */
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.text)
	}

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
}
