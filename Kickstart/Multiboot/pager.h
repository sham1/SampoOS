#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <SampoOS/Kernel/bootinfo.h>

enum page_type
{
	PAGE_TYPE_32BIT,
	PAGE_TYPE_64BIT,
};

enum page_perm
{
	PAGE_PERM_READ  = (1 << 0),
	PAGE_PERM_WRITE = (1 << 1),
	PAGE_PERM_EXEC  = (1 << 2),
};

bool initialize_pager(enum page_type type);

void map_page(uint64_t phys_addr, uint64_t virt_addr, enum page_perm perm_flags);

void pager_fill_bootinfo(struct sampo_bootinfo *info);
