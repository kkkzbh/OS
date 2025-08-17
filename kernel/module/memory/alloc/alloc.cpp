module;

#include <assert.h>
#include <string.h>
#include <stdio.h>

export module alloc;

export import :init;

import memory;
import schedule;
import pool;
import mutex;
import lock_guard;
import utility;

export auto get_vaddr(pool_flags pf,u32 cnt) -> void*;

export auto get_pool(pool_flags pf) -> auto&;

export auto get_kernel_pages(u32 pg_cnt) -> void*;

export auto get_a_page(pool_flags pf,u32 vaddr) -> void*;

export auto create_page_dir() -> u32*;

export auto allocate_pid() -> pid_t;

auto page_table_add(void* __vaddr,void* __page_phyaddr) -> void;

auto kernel_alloc_mtx = mutex{};
auto user_alloc_mtx = mutex{};
auto pid_lock = mutex{};

auto get_mutex(pool_flags pf) -> auto&
{
    using enum pool_flags;
    if(pf == KERNEL) {
        return kernel_alloc_mtx;
    }
    return user_alloc_mtx;
}

auto get_pool(pool_flags pf) -> auto&
{
    if(pf == pool_flags::KERNEL) {
        return kernel_pool;
    }
    return user_pool;
}

auto get_vaddr(pool_flags pf,u32 cnt) -> void*
{
    if(pf == pool_flags::KERNEL) {
        return kernel_vaddr.get(cnt);
    }
    auto cur = running_thread();
    auto vstart = cur->userprog_vaddr.get(cnt);
    // 0xc0000000 - PG_SIZE 作为用户3级栈 已经在start_process分配
    ASSERT((u32)vstart < (0xc0000000 - PG_SIZE));
    return vstart;
}

// 分配 pg_cnt 个页空间，成功返回虚拟地址，失败返回 nullptr
auto malloc_page(pool_flags pf,u32 pg_cnt) -> void*
{
    ASSERT(pg_cnt and pg_cnt < 3840); // 15MB封顶 总内存32MB 按对半划分 15MB封顶保险处理 实际上由于简化的向下取整，内核和用户都仅有3840片页
    auto vaddr_start = get_vaddr(pf,pg_cnt);
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
    auto lcg = lock_guard{ kernel_alloc_mtx };
    auto vaddr = malloc_page(pool_flags::KERNEL, pg_cnt);
    if (vaddr) {
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    }
    return vaddr;
}

// 在用户空间申请4k内存，并返回虚拟地址
auto get_user_pages(u32 pg_cnt) -> void*
{
    auto lcg = lock_guard{ user_alloc_mtx };
    auto vaddr = malloc_page(pool_flags::USER,pg_cnt);
    memset(vaddr,0,pg_cnt * PG_SIZE);
    return vaddr;
}

// 将地址vaddr与pf池中的物理地址关联，仅支持一页空间分配
auto get_a_page(pool_flags pf,u32 vaddr) -> void*
{
    using enum pool_flags;
    auto& pool = get_pool(pf);
    auto lcg = lock_guard{ get_mutex(pf) };
    auto cur = running_thread();
    
    // 先将虚拟地址位图置 1
    if(cur->pgdir != nullptr and pf == USER) {
        auto bi = cur->userprog_vaddr.trans(vaddr);
        cur->userprog_vaddr.set(bi,true);
    } else if(cur->pgdir == nullptr and pf == KERNEL) {
        auto bi = kernel_vaddr.trans(vaddr);
        kernel_vaddr.set(bi,true);
    } else {
        PANIC("get_a_page: not allow kernel alloc userpace or user alloc kernelspace by get_a_page");
    }

    auto page_phyaddr = pool.palloc();
    if(page_phyaddr == nullptr) {
        return nullptr;
    }

    page_table_add((void*)vaddr, page_phyaddr);
    return (void*)vaddr;
}

// 创建页目录表，将当前页表的表示内核空间的pde复制
// 成功则返回页目录的虚拟地址，否则返回 nullptr

auto create_page_dir() -> u32*
{
    // 用户进程的页表不能让用户直接访问到，故在内核空间中申请
    auto page_dir_vaddr = (u32*)get_kernel_pages(1);
    if(page_dir_vaddr == nullptr) {
        puts("create_page_dir: get_kernel_page failed!");
        return nullptr;
    }
    // 先复制页表，统一内核虚拟空间
    // page_dir_vaddr + 0x300*4 是内核页目录的第768项
    memcpy((void*)((u32)page_dir_vaddr + 0x300*4),(void*)(0xfffff000 + 0x300 * 4),1024);

    // 更新页目录地址
    auto new_page_dir_phy_addr = pgtable::addr_v2p((u32)page_dir_vaddr);
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1; // 让最后指向自己
    return page_dir_vaddr;

}

// 页表pte中 添加一个 虚拟地址与物理地址的映射
auto page_table_add(void* __vaddr,void* __page_phyaddr) -> void
{
    auto vaddr = reinterpret_cast<u32>(__vaddr);
    auto page_phyaddr = reinterpret_cast<u32>(__page_phyaddr);
    auto pde = pgtable::pde_ptr(vaddr); // 注意先确保 pde 建立完毕 才能访问 pte
    auto pte = pgtable::pte_ptr(vaddr);

    if(not pgtable::contains(pde)) { // 如果 pde 不存在
        // 内核物理内存池中分配一页内存
        auto pde_phyaddr = reinterpret_cast<u32>(kernel_pool.palloc());
        if (pde_phyaddr == 0) {
            PANIC("page_table_add: kernel_pool.palloc failed");
        }
        *pde = pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
        // 要将新分配的内存清 0 处理(防止*页表状态*混乱)，pte取高20位便是对应pde的物理地址
        memset(reinterpret_cast<void*>(reinterpret_cast<u32>(pte) & 0xfffff000),0,PG_SIZE);
    }

    ASSERT(not pgtable::contains(pte)); // 因为是在添加映射 所以pte就应该不存在
    *pte = page_phyaddr | PG_US_U | PG_RW_W | PG_P_1; // 页内存是否清空 交给外部处理
}

auto allocate_pid() -> pid_t
{
    auto static next_pid = 0;
    auto lcg = lock_guard{ pid_lock };
    return ++next_pid;
}