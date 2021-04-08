#pragma once

#include <SampoOS/Kernel/bootinfo.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

enum physmem_region_type
{
	PHYSMEM_REGION_TYPE_AVAILABLE,
	PHYSMEM_REGION_TYPE_RESERVED,
	PHYSMEM_REGION_TYPE_ACPI_RECLAIM,
	PHYSMEM_REGION_TYPE_NVS,
	PHYSMEM_REGION_TYPE_BAD_MEM,
};

struct physmem_region
{
	uintptr_t addr; // <- Page granuality.
	size_t len; // <- Multiples of PAGE_SIZE.

	uint8_t *alloc_map; // Bitmap describing allocated and free regions.

	enum physmem_region_type type;
};

void init_memory_manager(struct sampo_bootinfo *bootinfo);

enum virt_map_perm
{
	VIRT_MAP_READ = (1 << 0),
	VIRT_MAP_WRITE = (1 << 1),
	VIRT_MAP_EXEC = (1 << 2),
};

void *virt_map_pages_kernel_end(uintptr_t physical_page_addr,
				size_t page_count,
				enum virt_map_perm mapping_perms);
