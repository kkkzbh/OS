module;

#include <string.h>
#include <interrupt.h>
#include <assert.h>
#include <stdio.h>

export module malloc;

import utility;
import schedule;
import arena;
import pool;
import lock_guard;
import alloc;
import memory;

export auto malloc(size_t size) -> void*;

auto malloc(size_t size) -> void*
{
    auto cur_thread = running_thread();
    auto pf = pool_flags{};
    auto desc = (mem_block_desc*){};
    auto& pool = [&] -> auto& {
        if(cur_thread->pgdir == nullptr) { // 内核线程
            pf = pool_flags::KERNEL;
            desc = k_block_descs;
            return kernel_pool;
        } else {
            pf = pool_flags::USER;
            desc = cur_thread->u_block_desc;
            return user_pool;
        }
    }();

    if(size == 0 or size >= pool.size()) { // 内存范围不合法
        return nullptr;
    }
    auto lcg = lock_guard{ get_mutex(pf) };

    if(size > 1024) { // 属于大块内存 分配页框
        auto page_cnt = div_ceil(size + sizeof(arena),PG_SIZE);
        auto a = (arena*)malloc_page(pf,page_cnt);
        if(a == nullptr) {
            return nullptr;
        }
        memset(a,0,page_cnt * PG_SIZE);
        *a = (arena) {
            .desc = nullptr,
            .cnt = page_cnt,
            .large = true
        };
        return (void*)(a + 1); // 跨过 arena 大小，返回剩余内存
    }
    // 小块内存在mem_block_desc中去寻找适合的大小
    auto i = 0;
    for(; i != DESC_CNT; ++i) {
        if(size <= desc[i].block_size) {
            break;
        }
    }
    // 如果 mem_block_desc 的 free_list 没有可用的 mem_block
    // 创建新的 arena 提供 mem_block
    auto& list = desc[i].free_list;
    if(list.empty()) {
        auto a = (arena*)malloc_page(pf,1);
        if(a == nullptr) {
            return nullptr;
        }
        memset(a,0,PG_SIZE);
        *a = (arena) {
            .desc = &desc[i],
            .cnt = desc[i].block_per_arena,
            .large = false
        };
        auto old_status = intr_disable(); // 关中断
        // 将 arena 拆分为内存块，加到free_list中
        for(auto k = 0; k != desc[i].block_per_arena; ++k) {
            auto b = a->block(k);
            ASSERT(not list.contains(&b->free_elem));
            list.push_back(&b->free_elem);
        }
        intr_set_status(old_status);
    }

    // 开始分配内存块
    auto block = list.front();
    list.pop_front();
    auto b = (mem_block*)((u32)block - (u32)(&((mem_block*)0)->free_elem));
    auto a = ofarena(b);
    --a->cnt;
    return (void*)b;
}
