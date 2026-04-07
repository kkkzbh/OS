module;

#include <stdio.h>
#include <interrupt.h>
#include <assert.h>

export module ioqueue;

import utility;
import task;
import lock_guard;
import mutex;
import schedule;

// 循环队列
export struct ioqueue
{

    auto constexpr static bufsize = 64;

    struct iterator
    {

        auto operator*(this auto& self) -> decltype(auto)
        {
            return self.it;
        }

        auto operator++() -> iterator&
        {
            ++it;
            if(it == bufsize) {
                it = 0;
            }
            return *this;
        }

        auto friend operator==(iterator x,iterator y) -> bool = default;

        i32 it;
    };

    auto init() -> void
    {
        mtx.init();
        producer = nullptr;
        consumer = nullptr;
        for(auto& c : buf) {
            c = '\0';
        }
        head = {};
        tail = {};
    }

    auto full() const -> bool
    {
        assert_initialized();
        ASSERT(intr_get_status() == INTR_OFF);
        return ++end() == begin();
    }

    auto empty() const -> bool
    {
        assert_initialized();
        ASSERT(intr_get_status() == INTR_OFF);
        return begin() == end();
    }

    auto begin() const -> iterator
    {
        assert_initialized();
        return tail;
    }

    auto end() const -> iterator
    {
        assert_initialized();
        return head;
    }

    auto static wait(task*& waiter) -> void
    {
        ASSERT(waiter == nullptr);
        waiter = running_thread();
        thread_block(thread_status::blocked);
    }

    auto static wakeup(task*& waiter) -> void
    {
        ASSERT(waiter != nullptr);
        thread_unblock(waiter);
        waiter = nullptr;
    }

    auto get() -> char
    {
        assert_initialized();
        ASSERT(intr_get_status() == INTR_OFF);
        while(empty()) {
            auto lcg = lock_guard{ mtx };
            wait(consumer);
        }
        auto byte = buf[*tail];
        ++tail;
        if(producer) { // 如果有睡眠的producer
            wakeup(producer);
        }
        return byte;
    }

    auto put(char byte) -> void
    {
        assert_initialized();
        ASSERT(intr_get_status() == INTR_OFF);
        while(full()) {
            auto lcg = lock_guard{ mtx };
            wait(producer);
        }
        buf[*head] = byte;
        ++head;
        if(consumer) { // 如果有睡眠的consumer
            wakeup(consumer);
        }
    }

    auto assert_initialized() const -> void
    {
        ASSERT(mtx.initialized());
    }

    mutex mtx;

    task* producer; // 生产者，该属性记录在这个缓冲区上睡眠的生产者

    task* consumer; // 消费者，同

    char buf[bufsize];
    iterator head; // 队首 写入
    iterator tail; // 队尾 读出
};
