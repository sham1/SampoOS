#pragma once

#include <SampoOS/Kernel/bootinfo.h>
#include <stdint.h>
#include <stdbool.h>

enum physmem_region_type
{
	PHYSMEM_REGION_TYPE_AVAILABLE,
	PHYSMEM_REGION_TYPE_RESERVED,
	PHYSMEM_REGION_TYPE_ACPI_RECLAIM,
	PHYSMEM_REGION_TYPE_NVS,
	PHYSMEM_REGION_TYPE_BAD_MEM,
};

// Used for the implementation of the red-black tree.
enum physmem_region_color
{
	PHYSMEM_REGION_COLOR_BLACK,
	PHYSMEM_REGION_COLOR_RED,
};

// Physical memory regions are stored as nodes in a red-black tree.
struct physmem_region
{
	uintptr_t addr; // <- Page granuality.
	size_t len; // <- Multiples of PAGE_SIZE.

	uint8_t *alloc_map; // Bitmap describing allocated and free regions.

	struct physmem_region *left_child;
	struct physmem_region *right_child;

	enum physmem_region_type type;
	enum physmem_region_color color;
};
