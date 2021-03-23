#include "elf.h"
#include "util.h"
#include "serial.h"
#include <stddef.h>
#include "pager.h"
#include "pmm.h"
#include "string.h"

#define ET_EXEC 2

#define EM_386 3
#define EM_X86_64 62

enum elf_class
{
	ELF_CLASS_32 = 1,
	ELF_CLASS_64 = 2,
};

enum elf_endian
{
	ELF_ENDIAN_LITTLE = 1,
	ELF_ENDIAN_BIG = 2,
};

static struct elf_header
{
	uint8_t *base_ptr;

	enum elf_class class;
	enum elf_endian endianness;

	enum elf_arch arch;

	uint64_t entry_point;

	uint64_t prog_header_offset;
	uint16_t prog_header_count;
	size_t prog_header_len;
} header;

extern uint64_t program_entry_point;

static inline uint16_t
elf_read_u16(const uint8_t *bytes)
{
	if (header.endianness == ELF_ENDIAN_LITTLE)
	{
		return read_u16_le(bytes);
	}
	else
	{
		return read_u16_be(bytes);
	}
}

static inline uint32_t
elf_read_u32(const uint8_t *bytes)
{
	if (header.endianness == ELF_ENDIAN_LITTLE)
	{
		return read_u32_le(bytes);
	}
	else
	{
		return read_u32_be(bytes);
	}
}

static inline uint64_t
elf_read_u64(const uint8_t *bytes)
{
	if (header.endianness == ELF_ENDIAN_LITTLE)
	{
		return read_u64_le(bytes);
	}
	else
	{
		return read_u64_be(bytes);
	}
}

static inline uint64_t
elf_read_addr(const uint8_t *bytes, size_t *offset)
{
	if (header.class == ELF_CLASS_32)
	{
		uint64_t ret = elf_read_u32(&bytes[*offset]);
		*offset += 4;
		return ret;
	}
	else
	{
		uint64_t ret = elf_read_u64(&bytes[*offset]);
		*offset += 8;
		return ret;
	}
}

static inline uint64_t
elf_read_off(const uint8_t *bytes, size_t *offset)
{
	if (header.class == ELF_CLASS_32)
	{
		uint64_t ret = elf_read_u32(&bytes[*offset]);
		*offset += 4;
		return ret;
	}
	else
	{
		uint64_t ret = elf_read_u64(&bytes[*offset]);
		*offset += 8;
		return ret;
	}
}

static inline uint32_t
elf_read_word(const uint8_t *bytes, size_t *offset)
{
	uint32_t ret = elf_read_u32(&bytes[*offset]);
	*offset += 4;
        return ret;
}

static inline uint16_t
elf_read_half(const uint8_t *bytes, size_t *offset)
{
	uint16_t ret = elf_read_u16(&bytes[*offset]);
	*offset += 2;
        return ret;
}

bool
elf_initialize(uint8_t *elf_base_ptr)
{
	header.base_ptr = elf_base_ptr;

	size_t offset = 0;

	// First, let's validate that this is an ELF file.
	if (!(header.base_ptr[offset++] == 0x7F &&
	      header.base_ptr[offset++] == 'E' &&
	      header.base_ptr[offset++] == 'L' &&
	      header.base_ptr[offset++] == 'F'))
	{
		serial_write("File is not a valid ELF file\n");
		return false;
	}

	// Now we must see which class this is.
	if (header.base_ptr[offset] != ELF_CLASS_32 && header.base_ptr[offset] != ELF_CLASS_64)
	{
		serial_write("ELF file has unknown class\n");
		return false;
	}

	header.class = header.base_ptr[offset++];

	serial_printf("ELF file class: %d-bit\n", header.class == ELF_CLASS_32 ? 32 : 64);

	// And now the way multi-byte data is stored. Whether it's 2's complement and if it's little or big endian.
	if (header.base_ptr[offset] != ELF_ENDIAN_LITTLE && header.base_ptr[offset] != ELF_ENDIAN_BIG)
	{
		serial_write("ELF file has unknown endianness\n");
		return false;
	}

	header.endianness = header.base_ptr[offset++];

	// We can skip the rest of e_ident and focus on the other fields.
	// TODO: Actually see if the rest of e_ident is significant for us.

	// First, we must check that our file is actually an executable and not, say, a shared object.
	offset = 16;

	if (elf_read_half(header.base_ptr, &offset) != ET_EXEC)
	{
		serial_write("ELF file is not a proper executable\n");
		return false;
	}

	uint16_t e_machine = elf_read_half(header.base_ptr, &offset);
	if (e_machine != EM_386 && e_machine != EM_X86_64)
	{
		serial_write("ELF file is not compatible with the current architecture\n");
		serial_printf("Got %u\n", (unsigned int)e_machine);
		return false;
	}

	header.arch = e_machine == EM_386 ? ELF_ARCH_X86 : ELF_ARCH_AMD64;

	// We don't care about the object file format version.
	elf_read_word(header.base_ptr, &offset);

	// ...but we do care about the entrypoint.
	header.entry_point = elf_read_addr(header.base_ptr, &offset);
	// As well as the program header offset.
	header.prog_header_offset = elf_read_off(header.base_ptr, &offset);

	serial_printf("Program entry point is at 0x%x%x\n",
		      (uint32_t)(header.entry_point >> 32),
		      (uint32_t)(header.entry_point & 0xFFFFFFFF));

	// We don't care about the section offset.
	elf_read_off(header.base_ptr, &offset);

	// Neither x86 nor AMD64 have any flags we really care about, so skip those.
	elf_read_word(header.base_ptr, &offset);

	// We don't care about the size of the ELF header either.
	elf_read_half(header.base_ptr, &offset);

	// But we do care about the number and size of program headers.
	header.prog_header_len = elf_read_half(header.base_ptr, &offset);
	header.prog_header_count = elf_read_half(header.base_ptr, &offset);

	// Rest of the ELF header consists of section things used for linking
	// so we don't care about them.
	return true;
}

enum elf_arch
elf_get_arch(void)
{
	return header.arch;
}

bool
elf_expand(void)
{
#define PT_LOAD 1
#define PF_X 0x1 // Executable segment
#define PF_W 0x2 // Writable segment
#define PF_R 0x4 // Readable segment

	uintptr_t prog_header_base = (uintptr_t)((uint64_t)(uintptr_t)header.base_ptr + header.prog_header_offset);

	if (header.arch == ELF_ARCH_X86)
	{
	}
	else
	{
		for (size_t i = 0; i < header.prog_header_count; ++i)
		{
			uint8_t *prog_header = (void *)(prog_header_base + header.prog_header_len * i);
			size_t offset = 0;

			// First we read the type of the segment. If it's not loadable, just ignore it.
			if (elf_read_word(prog_header, &offset) != PT_LOAD)
			{
				continue;
			}

			// Next we have the flags.
			uint32_t flags = elf_read_word(prog_header, &offset);
			enum page_perm segment_perms = 0;
			if ((flags & PF_R) != 0) {
				segment_perms |= PAGE_PERM_READ;
			}

			if ((flags & PF_W) != 0) {
				segment_perms |= PAGE_PERM_WRITE;
			}

			if ((flags & PF_X) != 0) {
				segment_perms |= PAGE_PERM_EXEC;
			}

			// Now comes the file offset of the segment (how far into the file it is.)
			uint64_t f_off = elf_read_off(prog_header, &offset);
			// And here's the virtual address of this segment.
			uint64_t vaddr = elf_read_addr(prog_header, &offset);
			// We don't care about the physical address.
			elf_read_addr(prog_header, &offset);
			// Now comes the size of the segment within the file
			uint64_t file_len = elf_read_u64(&prog_header[offset]);
			offset += 8;
			// And here is the length the segment ought to have in memory.
			uint64_t mem_len = elf_read_u64(&prog_header[offset]);
			offset += 8;

			// We don't care about the alignment since we know that it's going to be page-aligned.
			// So now we can allocate pages for our segment.
			serial_printf("Segment of length 0x%x%x is going to get mapped to virtual address 0x%x%x\n",
				      (uint32_t)(mem_len >> 32),
				      (uint32_t)(mem_len & 0xFFFFFFFF),
				      (uint32_t)(vaddr >> 32),
				      (uint32_t)(vaddr & 0xFFFFFFFF));

			bool needs_rounding = false;
			size_t pages_needed;
			if ((mem_len & 0x0FFF) != 0) {
				needs_rounding = true;
			}

			pages_needed = ((size_t)(mem_len / 0x1000)) + (needs_rounding ? 1 : 0);
			void *segment_region =
			  pmm_allocate_region_with_type(pages_needed,
							SAMPO_BOOTINFO_MEMORY_REGION_TYPE_KERNEL);
			if (segment_region == NULL)
			{
				serial_write("Couldn't allocate segment!\n");
				return false;
			}

			serial_printf("Kernel segment is to be mapped to physical page 0x%x\n",
				      (uint32_t)(uintptr_t)(segment_region));

			serial_printf("Pages needed: %u\n", (uint32_t)pages_needed);

			memset(segment_region, 0, pages_needed * 0x1000);

			for (size_t i = 0; i < pages_needed; ++i)
			{
				map_page((uintptr_t)segment_region + i * 0x1000, vaddr + i * 0x1000, segment_perms);
			}

			void *file_segment = (void *)(uintptr_t)((uint64_t)(uintptr_t)header.base_ptr + f_off);
			memcpy(segment_region, file_segment, file_len);
		}
	}

#undef PF_R
#undef PF_W
#undef PF_X
#undef PT_LOAD

	program_entry_point = header.entry_point;

	return true;
}
