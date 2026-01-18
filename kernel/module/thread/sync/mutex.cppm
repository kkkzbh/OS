module;

#include <stdio.h>
#include <assert.h>

export module mutex;

import semaphore;
import task;
import schedule;

export struct mutex
{

    constexpr mutex() : holder(nullptr),sema(1),holder_repeat_nr(1)
    {
        // puts("******* mutux construction!!! ***********\n");
    }

    mutex(mutex const&) = delete;

    auto operator=(mutex const&) -> mutex& = delete;

    auto lock() -> void
    {
        auto cur = running_thread();
        if(holder != cur) {
            sema.acquire();
            holder = cur;
            holder_repeat_nr = 1;
            return;
        }
        ++holder_repeat_nr;
    }

    auto unlock() -> void
    {
        auto cur = running_thread();
        ASSERT(holder == cur);
        if(holder_repeat_nr > 1) {
            --holder_repeat_nr;
            return;
        }
        ASSERT(holder_repeat_nr == 1);
        holder = nullptr;
        holder_repeat_nr = 0;
        sema.release(); // 这个执行完后才算unlock 所以mutux的状态在这个操作之前搞完
    }

    task* holder;       // 锁的持有者
    semaphore sema;    // 利用二元信号量实现
    u32 holder_repeat_nr;   // 锁的持有者重复申请锁的次数
};

