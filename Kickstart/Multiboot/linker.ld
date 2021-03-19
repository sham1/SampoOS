ENTRY(_start)

SECTIONS
{
	/* This kickstart begins at 1MiB */
	. = 1M;

	kickstart_start = .;

	/* First let's prefer the multiboot header and .text-section */
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.text)
	}

	/* Read-only data */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* Initialized read-write data */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* Uninitialized read-write data and stack */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}

	/* Store the end of kickstart to figure out its length */
	kickstart_end = .;
}
