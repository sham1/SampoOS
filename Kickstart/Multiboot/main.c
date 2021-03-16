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

	for (struct multiboot_tag *tag = (struct multiboot_tag *) (addr + 8);
	     tag->type != MULTIBOOT_TAG_TYPE_END;
	     tag = (struct multiboot_tag *)(((uintptr_t)tag) + ((tag->size + 7) & ~7)))
	{
		if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
		{
			struct multiboot_tag_mmap *mmap_tag =
				(struct multiboot_tag_mmap *) tag;
			serial_write("Found memory map\n");

			for (multiboot_memory_map_t *entry =
				     (multiboot_memory_map_t *) mmap_tag->entries;
			     (void *) entry < (void *) (mmap_tag + mmap_tag->size);
			     entry = (multiboot_memory_map_t *)
				     (((uintptr_t)entry) + mmap_tag->entry_size))
			{
				// TODO: Parse, print and store
				serial_write("\tGot Entry\n");
			}
		}
	}

	// TODO: Parse multiboot headers.
}
