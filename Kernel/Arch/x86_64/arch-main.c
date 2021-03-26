#include <SampoOS/Kernel/bootinfo.h>
#include "memory-manager.h"

void
kernel_arch_init(struct sampo_bootinfo *info)
{
	init_memory_manager(info);
}
