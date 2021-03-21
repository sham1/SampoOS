#include "pager.h"
#include "pmm.h"
#include "serial.h"
#include "string.h"

void *top_page_structure;
static enum page_type page_type;

bool
initialize_pager(enum page_type type)
{
	page_type = type;
	if (page_type == PAGE_TYPE_32BIT)
	{
		// TODO: Implement
		serial_write("32-bit paging is not supported currently\n");
		return false;
	}
	else
	{
		// We are managing a 64-bit paging setup. We need to allocate PML4 alongside all other
		// levels and identity map the first 2MiB for the kernel's convenience.
		top_page_structure = pmm_allocate_region(4);
		if (!top_page_structure)
		{
			serial_write("Could not allocate page tables\n");
			return false;
		}

		uintptr_t pml4_addr = (uintptr_t)top_page_structure;
		memset(top_page_structure, 0, 0x4000);
		uintptr_t ident_pdp_addr = pml4_addr + 0x1000;
		uintptr_t ident_pd_addr = ident_pdp_addr + 0x1000;
		uintptr_t ident_pt_addr = ident_pd_addr + 0x1000;

		// The page shall be both readable and writable as well as present.
		uint64_t ident_pml4_entry = ((uint64_t)ident_pdp_addr) | 0x2 | 0x1;
		uint64_t ident_pdp_entry  = ((uint64_t)ident_pd_addr)  | 0x2 | 0x1;
		uint64_t ident_pd_entry   = ((uint64_t)ident_pt_addr)  | 0x2 | 0x1;

		memcpy((void *)pml4_addr, &ident_pml4_entry, sizeof(uint64_t));
		memcpy((void *)ident_pdp_addr, &ident_pdp_entry, sizeof(uint64_t));
		memcpy((void *)ident_pd_addr, &ident_pd_entry, sizeof(uint64_t));

		for (uintptr_t page = 0x0; page < 0x200000; page += 0x1000)
		{
			map_page(page, page, PAGE_PERM_READ | PAGE_PERM_WRITE | PAGE_PERM_EXEC);
		}
	}

	return true;
}

void
map_page(uint64_t phys_addr, uint64_t virt_addr, enum page_perm perm_flags)
{
	if (page_type == PAGE_TYPE_32BIT)
	{
	}
	else
	{
		size_t pt_idx = (virt_addr >> 12) & 0x1FF;
		size_t pd_idx = (virt_addr >> 21) & 0x1FF;
		size_t pdp_idx = (virt_addr >> 30) & 0x1FF;
		size_t pml4_idx = (virt_addr >> 39) & 0x1FF;

		void *entry_ptr = (void *)(((uintptr_t)top_page_structure) + 0x08 * pml4_idx);
		uint64_t pml4e;
		memcpy(&pml4e, entry_ptr, sizeof(pml4e));

		void *pdp_addr;

		if (pml4e == 0)
		{
			// This page structure has not been allocated yet. Do that and move on recursively.
			pdp_addr = pmm_allocate_region(1);
			memset(pdp_addr, 0, 0x1000);
			pml4e = ((uint64_t)(uintptr_t)pdp_addr) | 0x2 | 0x1;
			memcpy(entry_ptr, &pml4e, sizeof(pml4e));
		}
		else
		{
			pdp_addr = (void *)(((uintptr_t)pml4e) & (~0x0FFF));
		}

		entry_ptr = (void *)(((uintptr_t)pdp_addr) + 0x08 * pdp_idx);
		uint64_t pdpe;
		memcpy(&pdpe, entry_ptr, sizeof(pdpe));

		void *pd_addr;
		if (pdpe == 0)
		{
			// This page structure has not been allocated yet. Do that and move on recursively.
			pd_addr = pmm_allocate_region(1);
			memset(pd_addr, 0, 0x1000);
			pdpe = ((uint64_t)(uintptr_t)pd_addr) | 0x2 | 0x1;
			memcpy(entry_ptr, &pdpe, sizeof(pdpe));
		}
		else
		{
			pd_addr = (void *)(((uintptr_t)pdpe) & (~0x0FFF));
		}

		entry_ptr = (void *)(((uintptr_t)pd_addr) + 0x08 * pd_idx);
		uint64_t pde;
		memcpy(&pde, entry_ptr, sizeof(pde));

		void *pt_addr;
		if (pde == 0)
		{
			// This page structure has not been allocated yet. Do that and move on recursively.
			pt_addr = pmm_allocate_region(1);
			memset(pt_addr, 0, 0x1000);
			pde = ((uint64_t)(uintptr_t)pt_addr) | 0x2 | 0x1;
			memcpy(entry_ptr, &pde, sizeof(pde));
		}
		else
		{
			pt_addr = (void *)(((uintptr_t)pde) & (~0x0FFF));
		}

		entry_ptr = (void *)(((uintptr_t)pt_addr) + 0x08 * pt_idx);
		uint64_t pte;
		memcpy(&pte, entry_ptr, sizeof(pte));

		if (pte != 0)
		{
			uint64_t mapped_address = pte & ~0x0FFF;
			serial_printf("Virtual address 0x%x%x is already mapped to physical address 0x%x%x!\n",
				      (uint32_t)(virt_addr >> 32),
				      (uint32_t)(virt_addr & 0xFFFFFFFF),
				      (uint32_t)(mapped_address >> 32),
				      (uint32_t)(mapped_address & 0xFFFFFFFF));
			return;
		}

		// Set read-write flag accordingly.
		pte = phys_addr | 0x1;
		if ((perm_flags & PAGE_PERM_WRITE) != 0)
		{
			pte |= 0x2;
		}

		// Set execution flag accordingly.
		if ((perm_flags & PAGE_PERM_EXEC) == 0)
		{
			// Sets the NX flag.
			pte |= 0x80000000;
		}

		memcpy(entry_ptr, &pte, sizeof(pte));
	}
}

void
pager_fill_bootinfo(struct sampo_bootinfo *info)
{
	info->bootstrap_paging_structure_ptr = (uintptr_t) top_page_structure;
}
