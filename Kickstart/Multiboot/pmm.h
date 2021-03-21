#pragma once

#include <SampoOS/Kernel/bootinfo.h>
#include <stddef.h>

extern size_t memory_region_count;
extern struct sampo_bootinfo_memory_region memory_regions[1024];

void pmm_add_region(enum sampo_bootinfo_memory_region_type region_type,
		    uint64_t start, uint64_t end);

void pmm_reserve_memory_region(uintptr_t start, uintptr_t end);

void *pmm_allocate_region(size_t region_page_count);
void *pmm_allocate_region_with_type(size_t region_page_count,
				    enum sampo_bootinfo_memory_region_type type);

void pmm_deallocate(void *addr);

void pmm_fill_bootinfo(struct sampo_bootinfo *info);
