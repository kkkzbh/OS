module;

#include <assert.h>
#include <string.h>

export module memory:pgtable;

import :utility;
import :pool;

export auto page_table_add(void* __vaddr,void* __page_phyaddr) -> void;

export auto addr_v2p(u32 vaddr) -> u32;

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

// 页表pte中 添加一个 虚拟地址与物理地址的映射
auto page_table_add(void* __vaddr,void* __page_phyaddr) -> void
{
    auto vaddr = reinterpret_cast<u32>(__vaddr);
    auto page_phyaddr = reinterpret_cast<u32>(__page_phyaddr);
    auto pde = pde_ptr(vaddr); // 注意先确保 pde 建立完毕 才能访问 pte
    auto pte = pte_ptr(vaddr);
    if(not contains(pde)) { // 如果 pde 不存在
        // 内核物理内存池中分配一页内存
        auto pde_phyaddr = reinterpret_cast<u32>(kernel_pool.palloc());
        *pde = pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
        // 要将新分配的内存清 0 处理(防止*页表状态*混乱)，pte取高20位便是对应pde的物理地址
        memset(reinterpret_cast<void*>(reinterpret_cast<u32>(pte) & 0xfffff000),0,PG_SIZE);
    }
    ASSERT(not contains(pte)); // 因为是在添加映射 所以pte就应该不存在
    *pte = page_phyaddr | PG_US_U | PG_RW_W | PG_P_1; // 页内存是否清空 交给外部处理
}

// 得到虚拟地址映射的物理地址
auto addr_v2p(u32 vaddr) -> u32
{
    auto pte = pte_ptr(vaddr);
    // *pte的值是页表所在的物理页框地址
    // 去掉其低12位的页表项属性位 + vaddr的低12位

    return (*pte & 0xfffff000) + (vaddr & 0x00000fff);
}