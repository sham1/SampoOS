#include "multiboot2.h"
#include <stdint.h>
#include "util.h"
#include "string.h"
#include "serial.h"

void
kickstart_main(uint32_t addr, uint32_t magic)
{
	(void) addr;

	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
	{
		return;
	}

	if (!serial_init())
	{
		return;
	}

	serial_write("Multiboot magic successful\n");

	// TODO: Parse multiboot headers.
}
