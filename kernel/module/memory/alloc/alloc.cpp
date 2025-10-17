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
export auto get_mutex(pool_flags pf) -> auto&;
export auto malloc_page(pool_flags pf,u32 pg_cnt) -> void*;
export auto mfree_page(pool_flags pf,void* _vaddr,size_t pg_cnt) -> void;
export auto fork_pid() -> pid_t;
export auto get_a_page_without_op_vaddr_bitmap(pool_flags pf,u32 vaddr) -> void*;

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

// 去掉虚拟地址 vaddr 的映射，只去掉pte，不动pde
auto page_table_pte_remove(u32 vaddr) -> void
{
    auto pte = pgtable::pte_ptr(vaddr);
    *pte &= ~PG_P_1; // P 位置0
    asm volatile("invlpg %0" : : "m"(vaddr) : "memory"); // 更新 tlb
}

// 虚拟地址池中释放以 _vaddr 起始的连续pg_cnt个虚拟页地址
auto vaddr_remove(pool_flags pf,void* _vaddr,size_t pg_cnt) -> void
{
    auto vaddr = (u32)_vaddr;
    using enum pool_flags;
    if(pf == KERNEL) { // 虚拟内存池
        auto bi = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        kernel_vaddr.set(bi,bi + pg_cnt,false);
    } else {
        auto cur_thread = running_thread();
        auto bi = (vaddr - cur_thread->userprog_vaddr.vaddr_start) / PG_SIZE;
        cur_thread->userprog_vaddr.set(bi,bi + pg_cnt,false);
    }
}

// 回收物理页地址
auto pfree(u32 pg_phy_addr) -> void
{
    if(pg_phy_addr >= user_pool.phy_addr_start) { // 用户物理内存
        user_pool.set (
            (pg_phy_addr - user_pool.phy_addr_start) / PG_SIZE,
            false
        );
    } else {
        kernel_pool.set (
            (pg_phy_addr - kernel_pool.phy_addr_start) / PG_SIZE,
            false
        );
    }
}

// 释放以虚拟地址vaddr为起始的 cnt 个物理页框
auto mfree_page(pool_flags pf,void* _vaddr,size_t pg_cnt) -> void
{
    auto vaddr = (i32)_vaddr;
    ASSERT(pg_cnt >= 1 and vaddr % PG_SIZE == 0);
    auto pg_phy_addr = pgtable::addr_v2p(vaddr); // vaddr对应的物理地址
    // 确保释放的物理内存在 低端1MB+1KB的页目录 + 1KB大小的页表地址范围外
    ASSERT(pg_phy_addr % PG_SIZE == 0 and pg_phy_addr >= 0x102000);
    if(pg_phy_addr >= user_pool.phy_addr_start) { // 用户内存池
        vaddr -= PG_SIZE;
        for(auto i = 0; i != pg_cnt; ++i) {
            vaddr += PG_SIZE;
            pg_phy_addr = pgtable::addr_v2p(vaddr);
            // 确保物理内存始终属于用户内存池
            ASSERT(pg_phy_addr % PG_SIZE == 0 and pg_phy_addr >= user_pool.phy_addr_start);
            // 先把物理页框还到内存池
            pfree(pg_phy_addr);
            // 页表中清除虚拟地址所在的页表项pte
            page_table_pte_remove(vaddr);
        }
        // 回收虚拟地址
        vaddr_remove(pf,_vaddr,pg_cnt);
    } else { // 内核内存池
        vaddr -= PG_SIZE;
        for(auto i = 0; i != pg_cnt; ++i) {
            vaddr += PG_SIZE;
            pg_phy_addr = pgtable::addr_v2p(vaddr);
            // 始终确保物理内存属于内核内存池
            ASSERT(pg_phy_addr % PG_SIZE == 0 and pg_phy_addr >= kernel_pool.phy_addr_start and pg_phy_addr < user_pool.phy_addr_start);
            // 先回收物理页框至内存池
            pfree(pg_phy_addr);
            // 页表中清除虚拟地址的页表项pte
            page_table_pte_remove(vaddr);
        }
        // 回收虚拟地址
        vaddr_remove(pf,_vaddr,pg_cnt);
    }
}

auto allocate_pid() -> pid_t
{
    auto static next_pid = 0;
    auto lcg = lock_guard{ pid_lock };
    return ++next_pid;
}

// 单纯就是形式作用，原版因为allocate_pid是静态函数，但是我这里不是，为了形式还是添加
auto fork_pid() -> pid_t
{
    return allocate_pid();
}

// 安装1页大小的vaddr，专门针对fork时虚拟地址位图无需操作的情况
auto get_a_page_without_op_vaddr_bitmap(pool_flags pf,u32 vaddr) -> void*
{
    using enum pool_flags;
    auto& pool = get_pool(pf);
    auto lcg = lock_guard{ get_mutex(pf) };

    auto page_phyaddr = pool.palloc();
    if(page_phyaddr == nullptr) {
        return nullptr;
    }

    page_table_add((void*)vaddr, page_phyaddr);
    return (void*)vaddr;
}