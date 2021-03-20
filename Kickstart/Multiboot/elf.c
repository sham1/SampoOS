#include "elf.h"
#include "util.h"
#include "serial.h"
#include <stddef.h>

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
