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
    auto init(u8 desire) -> void
    {
        value = desire;
        waiters.init();
    }

    auto operator=(semaphore const&) -> semaphore& = delete;

    auto acquire() -> void
    {
        assert_initialized();
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

    auto try_acquire() -> bool
    {
        assert_initialized();
        auto old_status = intr_disable();
        if(value == 0) {
            intr_set_status(old_status);
            return false;
        }
        --value;
        intr_set_status(old_status);
        return true;
    }

    auto release() -> void
    {
        assert_initialized();
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

    auto assert_initialized() const -> void
    {
        ASSERT(initialized());
    }

    auto initialized() const -> bool
    {
        return waiters.is_initialized();
    }

    u8 value;
    list waiters;
};
