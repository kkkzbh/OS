module;

#include <assert.h>
#include <stdio.h>

export module pool;

import bitmap;
import memory;
import mutex;
import lock_guard;

// 虚拟内存池
export struct virtual_addr : bitmap
{
    auto init(char* addr,size_t sz,u32 vstart) -> void
    {
        bits = (u8*)addr;
        this->sz = sz;
        vaddr_start = vstart;
    }

    // 在虚拟内存池中申请 pg_cnt 个虚拟页
    // 成功返回虚拟起始地址 失败返回 nullptr
    auto get(u32 pg_cnt) -> void*
    {
        auto start = scan(pg_cnt);
        if(not start) {
            return nullptr;
        }
        set(*start,*start + pg_cnt,true);
        auto vaddr = vaddr_start + *start * PG_SIZE;
        return reinterpret_cast<void*>(vaddr);
    }

    using bitmap::set;

    // 将虚拟地址 转换为对应的位的位置
    [[nodiscard]]
    auto trans(u32 vaddr) const -> size_t
    {
        ASSERT(vaddr >= vaddr_start);
        return (vaddr - vaddr_start) / PG_SIZE;
    }

    // 将 vaddr对应的位图位 置1
    auto set(u32 vaddr) -> void
    {
        set(trans(vaddr),true);
    }

    u32 vaddr_start;
};

// 物理内存池
struct pool : bitmap
{
    auto init(char* addr,size_t sz,u32 pstart,size_t psize) -> void
    {
        bits = (u8*)addr;
        this->sz = sz;
        phy_addr_start = pstart;
        pool_size = psize;
    }

    // 分配一个物理页
    // 成功返回物理地址，失败返回 nullptr
    auto palloc() -> void*
    {
        auto lcg = lock_guard{ mtx };
        auto start = scan(1);
        if(not start) {
            return nullptr;
        }
        set(*start,true);
        auto page_phyaddr = phy_addr_start + *start * PG_SIZE;
        return reinterpret_cast<void*>(page_phyaddr);
    }

    [[nodiscard]]
    auto size() const -> size_t
    {
        return pool_size;
    }

    u32 phy_addr_start;
    u32 pool_size;
    mutex mtx;
};

export auto kernel_pool = pool{};          // 内核物理内存池位图
export auto user_pool = pool{};            // 用户物理内存池位图
export auto kernel_vaddr = virtual_addr{}; // 内核虚拟地址池位图
