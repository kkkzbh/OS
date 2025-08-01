

module memory:pool;

import bitmap;
import :utility;

// 虚拟内存池
struct virtual_addr : bitmap
{
    auto init(char* addr,size_t sz,u32 vstart) -> void
    {
        bits = addr;
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

    u32 vaddr_start;
};

// 物理内存池
struct pool : bitmap
{
    auto init(char* addr,size_t sz,u32 pstart,size_t psize) -> void
    {
        bits = addr;
        this->sz = sz;
        phy_addr_start = pstart;
        pool_size = psize;
    }

    // 分配一个物理页
    // 成功返回物理地址，失败返回 nullptr
    auto palloc() -> void*
    {
        auto start = scan(1);
        if(not start) {
            return nullptr;
        }
        set(*start,1);
        auto page_phyaddr = phy_addr_start + *start * PG_SIZE;
        return reinterpret_cast<void*>(page_phyaddr);
    }

    u32 phy_addr_start;
    u32 pool_size;
};

auto kernel_pool = pool{};          // 内核物理内存池位图
auto user_pool = pool{};            // 用户物理内存池位图
auto kernel_vaddr = virtual_addr{}; // 内核虚拟地址池位图
auto user_vaddr = virtual_addr{};



auto get_pool(pool_flags pf) -> pool&
{
    if(pf == pool_flags::KERNEL) {
        return kernel_pool;
    }
    return user_pool;
}

auto get_vaddr(pool_flags pf) -> virtual_addr&
{
    if(pf == pool_flags::KERNEL) {
        return kernel_vaddr;
    }
    return user_vaddr;
}