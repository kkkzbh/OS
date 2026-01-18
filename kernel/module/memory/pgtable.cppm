module;

#include <assert.h>
#include <string.h>
#include <stdio.h>

export module memory:pgtable;

import memory.utility;

namespace pgtable
{
    export auto pde_idx(u32 addr) -> u32;

    export auto pte_idx(u32 addr) -> u32;

    export auto pte_ptr(u32 vaddr) -> u32*;

    export auto pde_ptr(u32 vaddr) -> u32*;

    export auto contains(u32* addr) -> bool;

    export auto addr_v2p(u32 vaddr) -> u32;
}

namespace pgtable
{
    auto pde_idx(u32 addr) -> u32
    {
        return (addr & 0xffc00000) >> 22;
    }

    auto pte_idx(u32 addr) -> u32
    {
        return (addr & 0x003ff000) >> 12;
    }

    // 以下两个函数是 返回vaddr所在页表(页目录)的 虚拟地址 是逆向操作
    auto pte_ptr(u32 vaddr) -> u32*
    {
        // 0xffc00000
        // PDE = PDE[1023]  PDE[vaddr高10位] + vaddr中间10位索引
        return reinterpret_cast<u32*> (
            0xffc00000 + ((vaddr & 0xffc00000) >> 10) + pte_idx(vaddr) * 4
        );
    }

    auto pde_ptr(u32 vaddr) -> u32*
    {
        // 0xfffff000
        // PDE[1023][1023] = PDE 页目录表的地址 + vaddr高10位索引
        return reinterpret_cast<u32*> (
            0xfffff000 + pde_idx(vaddr) * 4 // 页目录项 页表项都是4字节
        );
    }

    auto contains(u32* addr) -> bool
    {
        // 判断pde和pte的 P 位 存在位
        return *addr & 0x00000001;
    }

    // 得到虚拟地址映射的物理地址
    auto addr_v2p(u32 vaddr) -> u32
    {
        auto pte = pte_ptr(vaddr);
        // *pte的值是页表所在的物理页框地址
        // 去掉其低12位的页表项属性位 + vaddr的低12位

        return (*pte & 0xfffff000) + (vaddr & 0x00000fff);
    }
}