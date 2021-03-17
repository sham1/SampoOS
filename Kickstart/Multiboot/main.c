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
			     (uintptr_t) entry < ((uintptr_t)(mmap_tag) + mmap_tag->size);
			     entry = (multiboot_memory_map_t *)
				     (((uintptr_t)entry) + mmap_tag->entry_size))
			{
				const char *type;
				switch (entry->type)
				{
				case MULTIBOOT_MEMORY_AVAILABLE:
					type = "Available";
					break;
				case MULTIBOOT_MEMORY_RESERVED:
					type = "Reserved";
					break;
				case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
					type = "ACPI Reclaimable";
					break;
				case MULTIBOOT_MEMORY_NVS:
					type = "ACPI NVS Memory";
					break;
				case MULTIBOOT_MEMORY_BADRAM:
					type = "Bad RAM";
					break;
				default:
					type = "Unknown";
					break;
				}
				// TODO: Parse, print and store
				serial_write("\tGot Entry\n");
				serial_printf("\t\tAddr: 0x%x%x - Length: 0x%x%x - Type: %s\n",
					      (uint32_t)(entry->addr >> 32),
					      (uint32_t)(entry->addr & 0xFFFFFFFF),
					      (uint32_t)(entry->len >> 32),
					      (uint32_t)(entry->len & 0xFFFFFFFF),
					      type);
			}
		}
		else if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER)
		{
			serial_write("Has framebuffer\n");
			struct multiboot_tag_framebuffer *fb_tag = (struct multiboot_tag_framebuffer *) tag;

			switch (fb_tag->common.framebuffer_type)
			{
			case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
				serial_write("\tType - indexed color\n");
				break;
			case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
				serial_write("\tType - RGB color\n");
				break;
			case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
				serial_write("\tType - Text mode\n");
				break;
			default:
				serial_write("\tType - unknown\n");
				break;
			}

			serial_printf("\tWidth - %u\n", fb_tag->common.framebuffer_width);
			serial_printf("\tHeight - %u\n", fb_tag->common.framebuffer_height);
			serial_printf("\tBits per pixel - %u\n", fb_tag->common.framebuffer_bpp);
			serial_printf("\tPitch - %u\n", fb_tag->common.framebuffer_pitch);
			serial_printf("\tAddress - 0x%x%x\n",
				      (uint32_t)(fb_tag->common.framebuffer_addr >> 32),
				      (uint32_t)(fb_tag->common.framebuffer_addr & 0xFFFFFFFF));
		}
	}

	// TODO: Parse multiboot headers.
}
