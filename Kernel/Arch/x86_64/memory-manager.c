#include <SampoOS/Kernel/memman.h>
#include "memory-manager.h"

const size_t PAGE_SIZE = 0x1000;

#define page_entry_to_physaddr(e) ((e) & UINT64_C(0x000FFFFFFFFFF000))
#define virtaddr_to_pml4e_idx(addr) ((((uintptr_t)(addr)) >> 39) & 0x1FF)
#define virtaddr_to_pdpe_idx(addr) ((((uintptr_t)(addr)) >> 30) & 0x1FF)
#define virtaddr_to_pde_idx(addr) ((((uintptr_t)(addr)) >> 21) & 0x1FF)
#define virtaddr_to_pte_idx(addr) ((((uintptr_t)(addr)) >> 12) & 0x1FF)

#define get_next_aligned_addr(addr) ((((uintptr_t)(addr)) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1))

static const uint64_t PAGE_PRESENT = UINT64_C(1) << 0;
static const uint64_t PAGE_WRITABLE = UINT64_C(1) << 1;
static const uint64_t NX_BIT = UINT64_C(1) << 63;

extern uint8_t kern_begin[];
extern uint8_t kern_end[];

uintptr_t kernel_end_addr;

static size_t physmem_len = 0;
static struct physmem_region *physmap = NULL;

const size_t FRACTAL_MAP_PML4_IDX = 510;
static const uintptr_t KERN_PT_BASE = UINT64_C(0xFFFF000000000000) + (FRACTAL_MAP_PML4_IDX << 39);
static const uintptr_t KERN_PD_BASE = KERN_PT_BASE + (FRACTAL_MAP_PML4_IDX << 30);
static const uintptr_t KERN_PDP_BASE = KERN_PD_BASE + (FRACTAL_MAP_PML4_IDX << 21);
static const uintptr_t KERN_PML4_BASE = KERN_PDP_BASE + (FRACTAL_MAP_PML4_IDX << 12);

static inline uint64_t *
get_pml4_from_addr(void *p)
{
	(void) p;
	return (uint64_t *) KERN_PML4_BASE;
}

static inline uint64_t *
get_pdpt_from_addr(void *p)
{
	uintptr_t addr = (uintptr_t) p;

	return (uint64_t *) (KERN_PDP_BASE + ((addr >> 27) & 0x00001FF000));
}

static inline uint64_t *
get_pd_from_addr(void *p)
{
	uintptr_t addr = (uintptr_t) p;

	return (uint64_t *) (KERN_PD_BASE + ((addr >> 18) & 0x003FFFF000));
}

static inline uint64_t *
get_pt_from_addr(void *p)
{
	uintptr_t addr = (uintptr_t) p;

	return (uint64_t *) (KERN_PT_BASE + ((addr >> 9) & 0x7FFFFFF000));
}

static inline void
invalidate_page(void *p)
{
	asm volatile("invlpg (%0)" : : "b"(p) : "memory");
}

void
init_memory_manager(struct sampo_bootinfo *bootinfo)
{
	// Round to next page for kernel end
	kernel_end_addr = (((uintptr_t) kern_end) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);

	// Let's first find our bootstrap paging structures.
	uint64_t *bootstrap_kernel = (uint64_t *)(uintptr_t) bootinfo->bootstrap_paging_structure_ptr;

	// Let's now do the fractal mapping.
	bootstrap_kernel[FRACTAL_MAP_PML4_IDX] =
		NX_BIT | (uintptr_t) bootstrap_kernel | PAGE_WRITABLE | PAGE_PRESENT;

	// Now we must calculate how many "combined" memory regions there are.
	// This means that any adjacent SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE
	// and SAMPO_BOOTINFO_MEMORY_REGION_TYPE_ALLOCATED are counted as "one region".
	size_t combined_region_count = 0;

	// We can also count how many bits we need for a bitmap
	// for all the combined available regions.
	size_t bitmap_bit_count = 0;
	struct sampo_bootinfo_memory_region *bootinfo_memory_regions =
		(struct sampo_bootinfo_memory_region *)(uintptr_t)
		bootinfo->memory_map.memory_regions_ptr;

	for (size_t i = 0; i < bootinfo->memory_map.memory_regions_count; ++i)
	{
		struct sampo_bootinfo_memory_region *region = &bootinfo_memory_regions[i];
		// First we check if this region is either AVAILABLE or ALLOCATED.
		if ((region->type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE) ||
		    (region->type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_ALLOCATED))
		{
			size_t combined_end = region->addr_end;
			for (size_t j = i + 1;
			     j < bootinfo->memory_map.memory_regions_count;
			     ++j)
			{
				struct sampo_bootinfo_memory_region *peek_region =
					&bootinfo_memory_regions[j];
				// If the regions are combinable, try combining.
				// If not, we've reached the end of the combined region.
				if ((peek_region->type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE) ||
				    (peek_region->type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_ALLOCATED))
				{
					// Combinable regions must be adjacent
					// (the previous one must end at the start of the next one.)
					if (peek_region->addr_start == combined_end)
					{
						combined_end = peek_region->addr_end;
						++i;
					}
					else
					{
						break;
					}
				}
				else
				{
					break;
				}
			}

			// We have now gotten the entire combined region.
			// See how many bits we need for the bitmap for this region.
			//
			// Because all the regions are guaranteed to be multiples of PAGE_SIZE by
			// Kickstart, this will work out fine.
			size_t region_size = combined_end - region->addr_start;
			region_size /= PAGE_SIZE;

			// We must also round the amount of bits to the next multiple of 8,
			// so we can then see how many bytes we will need.
			// This is also useful for the purpose of deciding which portions
			// of the bitmap belong to which region, since
			// every region will begin from the beginning of its
			// own byte, and thus there will be padding.

			bitmap_bit_count += (region_size + 0x7) & (~0x7);
		}

		++combined_region_count;
	}

	// Now, we have to figure out how many bytes we will have in this bitmap.
	// Because of the rounding above, this will be trivial.
	size_t bitmap_byte_count = bitmap_bit_count / 8;

	// And now we can see how many pages we need to allocate for this.
	size_t bitmap_page_count = (bitmap_byte_count / PAGE_SIZE) + 1;

	// Now we can also figure out how many pages the combined regions will need, if in a tight array.
	size_t regions_page_count = ((combined_region_count * sizeof(struct physmem_region)) / PAGE_SIZE) + 1;

	size_t phys_manager_page_count = bitmap_page_count + regions_page_count;

	uintptr_t phys_manager_addr = 0x0;
	// Let's now discover a physical region large enough to hold both the array of regions and the bitmap.
	for (size_t i = 0; i < bootinfo->memory_map.memory_regions_count; ++i)
	{
		struct sampo_bootinfo_memory_region *region = &bootinfo_memory_regions[i];
		// We can only use AVAILABLE regions, since ALLOCATED still contain the bootinfo
		// and our page tables.
		if (region->type == SAMPO_BOOTINFO_MEMORY_REGION_TYPE_AVAILABLE)
		{
			// Let's make sure that it's not under 1MiB
			if (region->addr_start < 0x100000)
			{
				continue;
			}

			// Now we must see if this region has enough space.
			size_t potential_end = region->addr_start + (phys_manager_page_count * PAGE_SIZE);
			if (region->addr_end < potential_end)
			{
				continue;
			}

			// We have our region!
			phys_manager_addr = region->addr_start;
			break;
		}
	}

	if (phys_manager_addr == 0x0)
	{
		// TODO: Log this out somehow to the user.
		return;
	}

	void *phys_manager_ptr = virt_map_pages_kernel_end(phys_manager_addr,
							   phys_manager_page_count,
							   VIRT_MAP_READ | VIRT_MAP_WRITE);

	// TODO: Add entries properly
}

void *
virt_map_pages_kernel_end(uintptr_t physical_page_addr,
			  size_t page_count,
			  enum virt_map_perm mapping_perms)
{
	void *ret = (void *)kernel_end_addr;
	for (size_t i = 0; i < page_count; ++i){
		size_t pml4e_idx = virtaddr_to_pml4e_idx(kernel_end_addr);
		size_t pdpe_idx = virtaddr_to_pdpe_idx(kernel_end_addr);
		size_t pde_idx = virtaddr_to_pde_idx(kernel_end_addr);
		size_t pte_idx = virtaddr_to_pte_idx(kernel_end_addr);

		// We must now traverse to see if all the necessary page
		// tables are available. If not, we need to allocate more.
		//
		// NOTE: Since this is called during the initialization
		// of the physical memory manager before it has set up
		// its data structures, this could fail.
		uint64_t *pml4 = get_pml4_from_addr((void *)kernel_end_addr);
		uint64_t pml4e = pml4[pml4e_idx];

		// We know that since these are after the kernel, this will never
		// not be present, but check for consistency.
		if ((PAGE_PRESENT & pml4e) == 0)
		{
			// Should never happen.
		        return NULL;
		}

		uint64_t *pdpt = get_pdpt_from_addr((void *)kernel_end_addr);
		uint64_t pdpe = pdpt[pdpe_idx];
		if ((PAGE_PRESENT & pdpe) == 0)
		{
			// TODO: Allocate new physical page.
		}

		uint64_t *pd = get_pd_from_addr((void *)kernel_end_addr);
		uint64_t pde = pd[pde_idx];
		if ((PAGE_PRESENT & pde) == 0)
		{
			// TODO: Allocate new physical page.
		}

		uint64_t *pt = get_pt_from_addr((void *)kernel_end_addr);
		uint64_t pte = pt[pte_idx];
		if ((PAGE_PRESENT & pte) != 0)
		{
			// We are trying to allocate over something else.
			// Shouldn't happen!
			return NULL;
		}

		// Create page table entry and set proper permissions
		pte = physical_page_addr | PAGE_PRESENT;
		if ((mapping_perms & VIRT_MAP_WRITE) != 0)
		{
			pte |= PAGE_WRITABLE;
		}
		if ((mapping_perms & VIRT_MAP_EXEC) == 0)
		{
			pte |= NX_BIT;
		}

		// Set the page table entry, and flush
		pt[pte_idx] = pte;
		invalidate_page((void *)kernel_end_addr);

		physical_page_addr += PAGE_SIZE;
		kernel_end_addr += PAGE_SIZE;
	}

	return ret;
}
