#include "multiboot2.h"
#include <stdint.h>
#include "util.h"
#include "string.h"
#include "serial.h"

#include "SampoOS/Kernel/bootinfo.h"

extern uint8_t kickstart_start[];
extern uint8_t kickstart_end[];

size_t memory_region_count = 0;
struct sampo_bootinfo_memory_region *memory_regions;

void *kernel_elf_location = NULL;

void
kickstart_main(uint32_t addr, uint32_t magic)
{
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
	{
		return;
	}

	if (!serial_init())
	{
		return;
	}

	serial_write("Multiboot magic successful\n");

	uint64_t free_start_addr = (((uint64_t)(uintptr_t)kickstart_end) + 0x0FFF) & ~0x0FFF;
	serial_printf("First free page after Kickstart: 0x%x%x\n",
		      (uint32_t)(free_start_addr >> 32),
		      (uint32_t)(free_start_addr & 0xFFFFFFFF));

	memory_regions = (struct sampo_bootinfo_memory_region *)(uintptr_t) free_start_addr;

	bool found_kernel = false;

	for (struct multiboot_tag *tag = (struct multiboot_tag *) (addr + 8);
	     tag->type != MULTIBOOT_TAG_TYPE_END;
	     tag = (struct multiboot_tag *)(((uintptr_t)tag) + ((tag->size + 7) & ~7)))
	{
		if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
		{
			struct multiboot_tag_mmap *mmap_tag =
				(struct multiboot_tag_mmap *) tag;
			for (multiboot_memory_map_t *entry =
				     (multiboot_memory_map_t *) mmap_tag->entries;
			     (uintptr_t) entry < ((uintptr_t)(mmap_tag) + mmap_tag->size);
			     entry = (multiboot_memory_map_t *)
				     (((uintptr_t)entry) + mmap_tag->entry_size))
			{
				enum sampo_bootinfo_memory_region_type region_type;
				switch (entry->type)
				{
				case MULTIBOOT_MEMORY_AVAILABLE:
					region_type = SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE;
					break;
				case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
					region_type = SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RECLAIMABLE;
					break;
				case MULTIBOOT_MEMORY_NVS:
					region_type = SAMPO_BOOTINFO_MEMORY_REGION_TYPE_NVS;
					break;
				case MULTIBOOT_MEMORY_BADRAM:
					region_type = SAMPO_BOOTINFO_MEMORY_REGION_TYPE_BAD_MEM;
					break;
				case MULTIBOOT_MEMORY_RESERVED:
				default:
					region_type = SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RESERVED;
					break;
				}
				uint64_t start_addr = entry->addr;
				uint64_t end_addr = entry->addr + entry->len;

				// Round appropriately to proper page boundaries.
				if ((start_addr & 0x0FFF) != 0x0000)
				{
					if (region_type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE)
					{
						// Round "free memory" upwards.
						start_addr = (start_addr + 0x0FFF) & ~0xFFF;
					}
					else if (region_type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RESERVED)
					{
						// Round "reserved" memory downawards.
						start_addr &= ~0x0FFF;
					}
				}

				if ((end_addr & 0x0FFF) != 0x0000)
				{
					if (region_type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE)
					{
						// Round "free memory" downwards.
						end_addr &= ~0x0FFF;
					}
					else if (region_type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RESERVED)
					{
						// Round "reserved" memory upwards.
						end_addr = (end_addr + 0x0FFF) & ~0xFFF;
					}
				}

				memory_regions[memory_region_count].addr_start = start_addr;
				memory_regions[memory_region_count].addr_end = end_addr;
				memory_regions[memory_region_count].type = region_type;

				++memory_region_count;
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
		else if (tag->type == MULTIBOOT_TAG_TYPE_MODULE)
		{
			serial_write("Has module\n");

			struct multiboot_tag_module *mod = (struct multiboot_tag_module *) tag;
			serial_printf("\tLocation - 0x%x\n", mod->mod_start);
			serial_printf("\tLength - 0x%x\n", mod->mod_end - mod->mod_start);
			serial_printf("\tCommand line - %s\n", mod->cmdline);

			// TODO: Maybe support i686 kernel
			if (strcmp(mod->cmdline, "sampo-x86_64.bin") == 0)
			{
				serial_write("\tModule was kernel!\n");
				kernel_elf_location = (void *)(uintptr_t)mod->mod_start;
				found_kernel = true;
			}
		}
	}

	if (!found_kernel)
	{
		serial_write("Bootloader could not load the kernel. Halting!\n");
		return;
	}

	if (memory_region_count == 0)
	{
		serial_write("Got no memory regions from Multiboot. Halting!\n");
		return;
	}

	serial_printf("Parsed memory map (rounded to nearest page boundaries):\n");
	for (size_t i = 0; i < memory_region_count; ++i)
	{
		serial_printf("\tRegion start - 0x%x%x\n",
			      (uint32_t)(memory_regions[i].addr_start >> 32),
			      (uint32_t)(memory_regions[i].addr_start & 0xFFFFFFFF));
		serial_printf("\tRegion end   - 0x%x%x\n",
			      (uint32_t)(memory_regions[i].addr_end >> 32),
			      (uint32_t)(memory_regions[i].addr_end & 0xFFFFFFFF));
		serial_printf("\tRegion type  - %d\n",
			      memory_regions[i].type);
		serial_putchar('\n');
	}

	// TODO: Parse multiboot headers.
}
