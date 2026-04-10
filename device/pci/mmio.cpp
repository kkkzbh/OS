module;

#include <assert.h>

export module pci.mmio;

import alloc;
import utility;

export auto mmio_map(u32 phys_addr,size_t bytes) -> void*;

auto mmio_map(u32 phys_addr,size_t bytes) -> void*
{
    ASSERT(bytes > 0);
    return map_kernel_phys_range(phys_addr,bytes);
}
