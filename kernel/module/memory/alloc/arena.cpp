module;

#include <stdio.h>

export module arena;

import list;
import utility;


export struct mem_block
{
    list::node free_elem;
};

export struct mem_block_desc
{
    u32 block_size;         // 内存块大小
    u32 block_per_arena;    // 一个arena中可容纳mem_block的数量
    list free_list;         // 可用的mem_block链表
};


export auto constexpr DESC_CNT = 7; // 内存块描述符的个数 16 32 64 128 256 512 1024


export struct arena
{

    // 返回 arena 中第 idx 个内存块的地址
    auto block(size_t idx) -> mem_block*
    {
        return (mem_block*)((u32)this + sizeof(arena) + idx * desc->block_size);
    }

    mem_block_desc* desc;   // arena关联的mem_block_desc
    u32 cnt;
    bool large;     // large = true: cnt表示页框数 else 表示空闲mem_block数量
};

export mem_block_desc k_block_descs[DESC_CNT];

export auto kernel_block_desc_init() -> void;

export auto ofarena(mem_block* b) -> arena*;

// 返回内存块b所在的arena地址
auto ofarena(mem_block* b) -> arena*
{
    return (arena*)((u32)b & 0xfffff000);
}

// 为 malloc 做准备
export auto block_desc_init(mem_block_desc* desc_array) -> void
{
    u16 sz = 16;
    for(auto i = 0; i != DESC_CNT; ++i) {
        // 不应该 聚合初始化 desc_array 因为其成员list 不应该被拷贝构造 拷贝赋值！
        desc_array[i].block_size = sz;
        desc_array[i].block_per_arena = ((PG_SIZE - sizeof(arena)) / sz);
        sz *= 2;
    }
}

auto kernel_block_desc_init() -> void
{
    block_desc_init(k_block_descs);
}