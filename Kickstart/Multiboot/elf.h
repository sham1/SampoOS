#pragma once

#include <stdbool.h>
#include <stdint.h>

enum elf_arch
{
	ELF_ARCH_X86,
	ELF_ARCH_AMD64,
};

bool elf_initialize(uint8_t *elf_base_ptr);

enum elf_arch elf_get_arch(void);

bool elf_expand(void);
