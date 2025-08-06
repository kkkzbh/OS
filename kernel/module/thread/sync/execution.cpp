module;

#include <assert.h>
#include <interrupt.h>
#include <switch.h>

export module sync:execution;

import utility;
import :task;


export auto schedule() -> void;

export auto running_thread() -> task*;

export auto thread_block(thread_status stu) -> void;

export auto thread_unblock(task* pthread) -> void;

export thread_list thread_ready_list; // 线程就绪队列
export thread_list thread_all_list;   // 所有任务队列

// 获取当前线程的 pcb 指针
auto running_thread() -> task*
{
    u32 esp;
    asm ("mov %%esp, %0" : "=g"(esp));
    // 取整 得到PCB起始地址
    return reinterpret_cast<task*>(esp & 0xfffff000);
}

// 实现任务调度
auto schedule() -> void
{
    ASSERT(intr_get_status() == INTR_OFF);

    auto cur = running_thread();

    if(cur->stu == thread_status::running) {
        // 表示该线程是 cpu 时间片到了
        ASSERT(not thread_ready_list.contains(&cur->general_tag));
        thread_ready_list.push_back(&cur->general_tag);
        cur->ticks = cur->priority;
        cur->stu = thread_status::ready;
    } else {
        // 否则应该是某个事件导致其后续才能继续上cpu运行 不加入就绪队列
    }
    ASSERT(not thread_ready_list.empty());
    auto thread_tag = thread_ready_list.front();
    thread_ready_list.pop_front();
    auto next = find_task_by_general(thread_tag);
    next->stu = thread_status::running;
    switch_to(cur,next);    // 调度
}

// 线程将自己阻塞，状态标记为 stu (blocked waiting hanging)
auto thread_block(thread_status stu) -> void
{
    using enum thread_status;
    ASSERT(stu == blocked or stu == waiting or stu == hanging);
    auto old_status = intr_disable();
    auto cur_thread = running_thread();
    cur_thread->stu = stu;
    schedule();
    // 当阻塞被解除后 才继续运行下面的
    intr_set_status(old_status);
}

// 解除 pthread 的阻塞
auto thread_unblock(task* pthread) -> void
{
    auto old_status = intr_disable();
    auto stu = pthread->stu;
    using enum thread_status;
    ASSERT(stu == blocked or stu == waiting or stu == hanging);
    ASSERT(not thread_ready_list.contains(&pthread->general_tag));
    thread_ready_list.push_front(&pthread->general_tag); // 唤醒后放到前面，就让他先运行吧
    pthread->stu = ready;
    intr_set_status(old_status);
}