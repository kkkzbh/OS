module;

#include <stdio.h>
#include <interrupt.h>
#include <assert.h>

export module semaphore;

import list;
import schedule;
import task;
import utility;

export struct semaphore
{

    constexpr semaphore() = default;  // 使用未初始化的semaphore是未定义的，但是为了兼容C以及特殊的初始化方式不得不引入默认构造

    explicit constexpr semaphore(u8 desire)
    {
        init(desire);
        // puts(" ***** semaphore construction!! ****** \n");
    }

    auto constexpr init(u8 desire) -> void
    {
        value = desire;
    }

    semaphore(semaphore const&) = delete;

    auto operator=(semaphore const&) -> semaphore& = delete;

    auto acquire() -> void
    {
        auto old_status = intr_disable();
        while(value == 0) {
            // 唤醒且无信号量 那么这时候阻塞队列里不应该存在自己
            auto cur = running_thread();
            ASSERT(not waiters.contains(&cur->general_tag));
            waiters.push_back(&cur->general_tag);
            thread_block(thread_status::blocked);
        }
        // 直到或一开始存在可获得的信号
        --value;
        intr_set_status(old_status);
    }

    auto release() -> void
    {
        auto old_status = intr_disable();
        if(not waiters.empty()) {
            auto thread_tag = waiters.front();
            waiters.pop_front();
            auto thread_blocked = find_task_by_general(thread_tag);
            thread_unblock(thread_blocked);
        }
        ++value;
        intr_set_status(old_status);
    }

    u8 value;
    list waiters{};
};