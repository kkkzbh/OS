module;

#include <assert.h>

export module free;

import schedule;
import utility;
import pool;
import lock_guard;
import alloc;
import arena;
import memory;

export auto free(void* ptr) -> void;

auto free(void* ptr) -> void
{
    ASSERT(ptr != nullptr);
    if(ptr == nullptr) {
        return;
    }
    auto pf = pool_flags{};
    auto& pool = [&] -> auto& { // 判断是内核线程还是用户进程(线程)
        if(running_thread()->pgdir == nullptr) {
            ASSERT((u32)ptr >= K_HEAP_START);
            pf = pool_flags::KERNEL;
            return kernel_pool;
        }
        pf = pool_flags::USER;
        return user_pool;
    }();
    auto lcg = lock_guard{ get_mutex(pf) };
    auto b = (mem_block*)ptr;
    auto a = ofarena(b);
    if(a->desc == nullptr and a->large) { // 大内存 (> 1024字节)
        mfree_page(pf,a,a->cnt);
    } else { // 小内存块
        // 先把内存回收到 free_list
        auto& list = a->desc->free_list;
        list.push_back(&b->free_elem);
        // 判断这个 arena 中的内存块是否全部都是空闲，如果是就释放arena
        if(++a->cnt == a->desc->block_per_arena) {
            for(auto i = 0; i != a->desc->block_per_arena; ++i) {
                auto block = a->block(i);
                ASSERT(list.contains(&block->free_elem));
                list.erase(&block->free_elem);
            }
            mfree_page(pf,a,1);
        }
    }

}