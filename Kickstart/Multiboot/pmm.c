#include "pmm.h"
#include "string.h"
#include <stdbool.h>
#include "serial.h"

size_t memory_region_count = 0;
struct sampo_bootinfo_memory_region memory_regions[1024];

static void pmm_sort_regions(void);

void
pmm_add_region(enum sampo_bootinfo_memory_region_type region_type,
	       uint64_t start, uint64_t end)
{
	// Round appropriately to proper page boundaries.
	if ((start & 0x0FFF) != 0x0000)
	{
		if (region_type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE)
		{
			// Round "free memory" upwards.
			start = (start + 0x0FFF) & ~0xFFF;
		}
		else if (region_type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RESERVED)
		{
			// Round "reserved" memory downawards.
			start &= ~0x0FFF;
		}
	}

	if ((end & 0x0FFF) != 0x0000)
	{
		if (region_type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE)
		{
			// Round "free memory" downwards.
			end &= ~0x0FFF;
		}
		else if (region_type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RESERVED)
		{
			// Round "reserved" memory upwards.
			end = (end + 0x0FFF) & ~0xFFF;
		}
	}

	memory_regions[memory_region_count].addr_start = start;
	memory_regions[memory_region_count].addr_end = end;
	memory_regions[memory_region_count].type = region_type;

	++memory_region_count;

	pmm_sort_regions();
	// TODO: Maybe handle with possibly overlapping regions.
}

void
pmm_reserve_memory_region(uintptr_t start, uintptr_t end)
{
	if ((start & 0x0FFF) != 0x0000)
	{
		start &= ~0x0FFF;
	}

	if ((end & 0x0FFF) != 0x0000)
	{
		end = (end + 0x0FFF) & ~0xFFF;
	}

	// First we must check that the requested region falls within
	// an available region.
	// We shall assume that all adjacent regions of the same type have been
	// merged properly.
	bool is_valid_block = false;
	struct sampo_bootinfo_memory_region *containing_region = NULL;
	for (size_t i = 0; i < memory_region_count; ++i)
	{
		struct sampo_bootinfo_memory_region *region = &memory_regions[i];
		if (region->addr_start > start)
		{
			continue;
		}

		if (region->addr_end < end)
		{
			continue;
		}

		if (region->type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE)
		{
			is_valid_block = true;
			containing_region = region;
			break;
		}
	}
	if (!is_valid_block)
	{
		return;
	}

	struct sampo_bootinfo_memory_region *new_region = &memory_regions[memory_region_count];
	new_region->addr_start = (uint64_t)start;
	new_region->addr_end = (uint64_t)end;
        new_region->type = SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RESERVED;

	++memory_region_count;

	// Before sorting, we must deal with one more thing: How do these two regions overlap.
	// There are three possibilities:
	// 1) new_region starts at the same address as the containing_region.
	// 2) new_region ends at the same address as the containing_region.
	// 3) new_region is totally enclosed by containing_region.
	//
	// new_region covering containing_region totally is a special case.
	if (new_region->addr_start == containing_region->addr_start)
	{
		// Move the containing region's start to after the new region.
		containing_region->addr_start = new_region->addr_end;
	}
	else if (new_region->addr_end == containing_region->addr_end)
	{
		// Move the containing region's end to before the new region.
		containing_region->addr_end = new_region->addr_start;
	}
	else
	{
		// new_region is totally enclosed: split containing_region.
		memory_regions[memory_region_count].addr_start = new_region->addr_end;
		memory_regions[memory_region_count].addr_end = containing_region->addr_end;
		memory_regions[memory_region_count].type = containing_region->type;

		++memory_region_count;

		containing_region->addr_end = new_region->addr_start;
	}

	pmm_sort_regions();
}

void *
pmm_allocate_region(size_t region_page_count)
{
	if (region_page_count == 0)
	{
		return NULL;
	}

	uint64_t len = region_page_count * 0x1000;
	void *ret = NULL;
	// We want to search from all over 1MiB available regions a region that is available.
	for (size_t i = 0; i < memory_region_count; ++i)
	{
		struct sampo_bootinfo_memory_region *potential_region = &memory_regions[i];
		if (potential_region->addr_start < 0x100000)
		{
			// Is the region under 1 MiB? Skip.
			continue;
		}

		if (potential_region->type != SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE)
		{
			continue;
		}

		// Now we shall see if the region is long enough.
		uint64_t potential_end = potential_region->addr_start + len;
		if (potential_end > potential_region->addr_end)
		{
			continue;
		}

		// We have our region. Create the appropriate reserved region.
		memory_regions[memory_region_count].addr_start = potential_region->addr_start;
		memory_regions[memory_region_count].addr_end = potential_end;
		memory_regions[memory_region_count].type = SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RESERVED;

		++memory_region_count;

		ret = (void *)(uintptr_t)potential_region->addr_start;
		// Now make the used region shorter.
		potential_region->addr_start = potential_end;

		break;
	}

	if (ret)
	{
		// If we actually found an applicable region, sort.
		pmm_sort_regions();
	}

	return ret;
}

void
pmm_deallocate(void *addr)
{
	uint64_t page_addr = (uint64_t)(uintptr_t) addr;
	if ((page_addr & 0x0FFF) != 0x0000)
	{
		page_addr &= ~0x0FFF;
	}

	for (size_t i = 0; i < memory_region_count; ++i)
	{
		struct sampo_bootinfo_memory_region *potential_region = &memory_regions[i];
		if (potential_region->addr_start < page_addr)
		{
			continue;
		}

		if (potential_region->addr_start > page_addr)
		{
			// We missed our page. And because these are sorted, this will
			// not find anything.
			return;
		}

		// We now know that `potential_region` aligns with the address we passed.
		// Let's see if it is a reserved region.
		if (potential_region->type != SAMPO_BOOTINFO_MEMORY_REGION_TYPE_RESERVED)
		{
			// We are at a non-reserved region: we don't need to do anything.
			return;
		}

		potential_region->type = SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE;
		break;
	}

	// Now that we have freed the region, we must coalesce any adjacent available
	// regions together.
	for (size_t i = 0; i < memory_region_count; ++i)
	{
		struct sampo_bootinfo_memory_region *head = &memory_regions[i];
		if (head->type != SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE)
		{
			continue;
		}

		if (head->addr_start == head->addr_end)
		{
			continue;
		}

		for (size_t j = i + 1; j < memory_region_count; ++j)
		{
			struct sampo_bootinfo_memory_region *current = &memory_regions[j];

			if (current->type != SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE)
			{
				break;
			}

			if (current->addr_start == current->addr_end)
			{
				continue;
			}

			if (head->addr_end != current->addr_start)
			{
				break;
			}

			head->addr_end = current->addr_end;
			current->addr_start = current->addr_end;
		}
	}

	// Remove any invalidated regions.
	pmm_sort_regions();
}

static void
pmm_sort_regions(void)
{
	// First we see how many "invalid" regions there are (= regions of length 0).
	size_t invalid_count = 0;
	for (size_t i = 0; i < memory_region_count; ++i)
	{
		if (memory_regions[i].addr_start == memory_regions[i].addr_end)
		{
			++invalid_count;
		}
	}

	// Bubble sort the regions. While this will be O(n^2) at the worst case,
	// since this is only done at boot-time and the number of entries
	// will probably not be very large, it's not going to be that bad.
	for (size_t i = 0; i < memory_region_count; ++i)
	{
		for (size_t j = 0; j < memory_region_count - i - 1; ++j)
		{
			struct sampo_bootinfo_memory_region tmp;
			if (memory_regions[j].addr_start == memory_regions[j].addr_end)
			{
				// All the invalid regions get sorted to the end unconditionally.
				memcpy(&tmp, &memory_regions[j], sizeof(*memory_regions));
				memcpy(&memory_regions[j], &memory_regions[j + 1], sizeof(*memory_regions));
				memcpy(&memory_regions[j + 1], &tmp, sizeof(*memory_regions));
				continue;
			}

			if (memory_regions[j].addr_start > memory_regions[j + 1].addr_start)
			{
				memcpy(&tmp, &memory_regions[j], sizeof(*memory_regions));
				memcpy(&memory_regions[j], &memory_regions[j + 1], sizeof(*memory_regions));
				memcpy(&memory_regions[j + 1], &tmp, sizeof(*memory_regions));
			}
		}
	}

	memory_region_count -= invalid_count;
}
