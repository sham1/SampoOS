#pragma once

#include <stdint.h>

enum sampo_bootinfo_memory_region_type
{
	SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE,
	SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RESERVED,
	SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RECLAIMABLE,
	SAMPO_BOOTINFO_MEMORY_REGION_TYPE_NVS,
	SAMPO_BOOTINFO_MEMORY_REGION_TYPE_BAD_MEM,

	SAMPO_BOOTINFO_MEMORY_REGION_TYPE_ALLOCATED,
	SAMPO_BOOTINFO_MEMORY_REGION_TYPE_KERNEL,
};

struct sampo_bootinfo_memory_region
{
	uint64_t addr_start;
	uint64_t addr_end;
	enum sampo_bootinfo_memory_region_type type;
};

struct sampo_bootinfo
{
	struct
	{
		uint64_t memory_regions_ptr;
		uint64_t memory_regions_count;
	} memory_map;

	uint64_t bootstrap_paging_structure_ptr;
};
