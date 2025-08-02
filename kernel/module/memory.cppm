module;

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <global.h>

export module memory;

import bitmap;

auto constexpr PG_SIZE = u32{ 4096 };

// 0xc009f000 是内核主线程栈顶，0xc009e000是内核主线程的pcb
// 一个页框大小的位图可表示 128MB 内存，位图位置安排在 0xc009a000
// 则支持 4 个页框的位图 共 512MB
auto constexpr MEM_BITMAP_BASE = 0xc009a000;

// 跨过内核虚拟地址的低端1MB
auto constexpr K_HEAP_START = 0xc0100000;

enum struct pool_flags
{
    KERNEL,
    USER
};

auto constexpr PG_P_1   =   1;  // 页表项或页目录项 存在 属性位
auto constexpr PG_P_0   =   0;

auto constexpr PG_RW_R  =   0;  // R/W属性位，读/执行
auto constexpr PG_RW_W  =   2;  // 读/写/执行

auto constexpr PG_US_S  =   0;  // U/S 属性位值，系统级
auto constexpr PG_US_U  =   4;  // 用户级

auto pde_idx(u32 addr) -> u32
{
    return (addr & 0xffc00000) >> 22;
}

auto pte_idx(u32 addr) -> u32
{
    return (addr & 0x003ff000) >> 12;
}

auto pte_ptr(u32 vaddr) -> u32*
{
    // 0xffc00000
    // PDE = PDE[1023]  PDE[vaddr高10位] + vaddr中间10位索引
    return reinterpret_cast<u32*> (
        0xffc00000 + ((vaddr & 0xffc00000) >> 10) + pte_idx(vaddr) * 4
    );
}

// vaddr -> pde 指针
auto pde_ptr(u32 vaddr) -> u32*
{
    // 0xfffff000
    // PDE[1023][1023] = PDE 页目录表的地址 + vaddr高10位索引
    return reinterpret_cast<u32*> (
        0xfffff000 + pde_idx(vaddr) * 4 // 页目录项 页表项都是4字节
    );
}

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


void mem_pool_init(u32 all_mem)
{
    puts("   mem_pool_init start\n");
    auto page_table_size = PG_SIZE * 256;
    // 1页的页目录表 + (0,768)项指向同一个页表 + 剩余769~1022(254个) ([1023]pde指向页目录表) 共计256个
    auto used_mem = page_table_size + 0x100000; // 0x100000 低端1MB 内存
    auto free_mem = all_mem - used_mem;
    auto all_free_pages = static_cast<u16>(free_mem / PG_SIZE); // 向下取整，找空余页数 u16->65536 256MB内存以内
    auto kernel_free_pages = all_free_pages / 2;
    auto user_free_pages = all_free_pages - kernel_free_pages;

    // 为简化处理 不处理余数 在丢失部分内存的情况下换来不用越界检查

    // bitmap的长度 位图中的一位表示一页 按byte存储起来除 8
    auto kbm_length = kernel_free_pages / 8;
    auto ubm_length = user_free_pages / 8;

    // 内核内存池的起始地址
    auto kp_start = used_mem;
    // 用户内存池的起始地址
    auto up_start = kp_start + kernel_free_pages * PG_SIZE;

    // 初始化内核池与用户池
    // 内核的位图在约定的 MEM_BITMAP_BASE 后面紧跟用户位图
    kernel_pool.init (
          reinterpret_cast<char*>(MEM_BITMAP_BASE),
          kbm_length,
          kp_start,
          kernel_free_pages * PG_SIZE
    );

    user_pool.init (
        reinterpret_cast<char*>(MEM_BITMAP_BASE + kbm_length),
        ubm_length,
        up_start,
        user_free_pages * PG_SIZE
    );

    puts("      kernel_pool_bitmap_start: ");
    puthex(reinterpret_cast<int>(kernel_pool.bits));
    puts("  kernel_pool_phy_addr_start: ");
    puthex(kernel_pool.phy_addr_start);
    putchar('\n');
    puts("      user_pool_bitmap_start: ");
    puthex(reinterpret_cast<int>(user_pool.bits));
    puts("  user_pool_phy_addr_start: ");
    puthex(user_pool.phy_addr_start);
    putchar('\n');

    kernel_pool.clear();
    user_pool.clear();

    // 初始化内核虚拟地址位图

    kernel_vaddr.init (
        reinterpret_cast<char*>(MEM_BITMAP_BASE + kbm_length + ubm_length),
        kbm_length,  // 内核堆的虚拟地址 与内核的物理内存池 **大小** 保持一致
        K_HEAP_START
    );

    kernel_vaddr.clear();

    puts("   mem_pool_init done\n");

}

export auto c_mem_init() -> void
{
    puts("mem_init start\n");
    auto total_mem_bytes = *reinterpret_cast<u32*>(0xb00);
    mem_pool_init(total_mem_bytes);
    puts("mem_init done\n");
}