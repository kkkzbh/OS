module;

#include <assert.h>
#include <string.h>

export module memory:pgtable;

import :utility;
import :pool;

export auto get_kernel_pages(u32 pg_cnt) -> void*;



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

// 分配 pg_cnt 个页空间，成功返回虚拟地址，失败返回 nullptr
auto malloc_page(pool_flags pf,u32 pg_cnt) -> void*
{
    ASSERT(pg_cnt and pg_cnt < 3840); // 15MB封顶 总内存32MB 按对半划分 15MB封顶保险处理 实际上由于简化的向下取整，内核和用户都仅有3840片页
    auto& vaddr_pool = get_vaddr(pf);
    auto vaddr_start = vaddr_pool.get(pg_cnt);
    if(not vaddr_start) {
        return nullptr;
    }
    auto vaddr = reinterpret_cast<u32>(vaddr_start);
    auto cnt = pg_cnt;
    auto& pool = get_pool(pf);
    // 虚拟地址连续 物理内存可不连续 做逐个按页映射
    while(cnt--) {
        auto page_phyaddr = pool.palloc();
        if(not page_phyaddr) { // 内存已满
            // 回滚全部物理页 (地址回收功能)
            // !-----------------------!!将来实现!!------------------------------!
            return nullptr;
        }
        page_table_add(reinterpret_cast<void*>(vaddr),page_phyaddr);
        vaddr += PG_SIZE;
    }
    return vaddr_start;
}

// 从内核物理内存池申请 pg_cnt 页内存
// 成功返回虚拟地址，失败返回 nullptr
auto get_kernel_pages(u32 pg_cnt) -> void*
{
    auto vaddr = malloc_page(pool_flags::KERNEL,pg_cnt);
    if(vaddr) {
        memset(vaddr,0,pg_cnt * PG_SIZE);
    }
    return vaddr;
}