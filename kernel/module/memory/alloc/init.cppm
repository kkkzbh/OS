module;

#include <stdio.h>

export module alloc:init;

import memory;
import pool;
import arena;

export auto mem_init() -> void;


auto mem_pool_init(u32 all_mem) -> void
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

auto mem_init() -> void
{
    puts("mem_init start\n");
    auto total_mem_bytes = *reinterpret_cast<u32*>(0xb00);
    mem_pool_init(total_mem_bytes);
    kernel_block_desc_init();
    puts("mem_init done\n");
}