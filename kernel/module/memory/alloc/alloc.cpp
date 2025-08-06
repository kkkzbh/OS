module;

#include <assert.h>
#include <string.h>

export module alloc;

import memory;
import sync;

export auto get_vaddr(pool_flags pf,u32 cnt) -> void*;

export auto get_pool(pool_flags pf) -> auto&;

export auto get_kernel_pages(u32 pg_cnt) -> void*;

auto kernel_mtx = mutex{};
auto user_mtx = mutex{};

auto get_mutex(pool_flags pf) -> auto&
{
    using enum pool_flags;
    if(pf == KERNEL) {
        return kernel_mtx;
    }
    return user_mtx;
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
    ASSERT((u32)vstart < 0xc0000000 - PG_SIZE);
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
    auto vaddr = malloc_page(pool_flags::KERNEL,pg_cnt);
    if(vaddr) {
        memset(vaddr,0,pg_cnt * PG_SIZE);
    }
    return vaddr;
}

// 在用户空间申请4k内存，并返回虚拟地址
auto get_user_pages(u32 pg_cnt) -> void*
{
    auto lcg = lock_guard{ user_mtx };
    auto vaddr = malloc_page(pool_flags::USER,pg_cnt);
    memset(vaddr,0,pg_cnt * PG_SIZE);
    return vaddr;
}

// 将地址vaddr与pf池中的物理地址关联，仅支持一页空间分配
auto get_a_page(pool_flags pf,u32 vaddr) -> void*
{
    using enum pool_flags;
    auto& pool = get_pool(pf);
    auto& mtx = get_mutex(pf);
    auto lcg = lock_guard{ mtx };
    auto cur = running_thread();
    // 先将虚拟地址位图置 1

    // 如果是用户进程申请用户内存
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

    page_table_add((void*)vaddr,page_phyaddr);
    return (void*)vaddr;
}