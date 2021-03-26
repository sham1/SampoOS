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

extern uint8_t kern_begin;
extern uint8_t kern_end;

// Representes the physmap as a tree.
static struct physmem_region *physmap = NULL;

// Stores the physical nodes of the physmap tree, without being in logical order.
static size_t physmem_len = 0;
static struct physmem_region *physmap_alloc = NULL;

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
	// Let's first find our bootstrap paging structures.
	uint64_t *bootstrap_kernel = (uint64_t *)(uintptr_t) bootinfo->bootstrap_paging_structure_ptr;

	// Let's now do the fractal mapping.
	bootstrap_kernel[FRACTAL_MAP_PML4_IDX] =
		NX_BIT | (uintptr_t) bootstrap_kernel | PAGE_WRITABLE | PAGE_PRESENT;
}
