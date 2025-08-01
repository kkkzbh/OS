module;

#include <string.h>
#include <global.h>
#include <stdio.h>
#include <interrupt.h>
#include <assert.h>

export module thread:exec;

import :utility;
import memory;
import :stack;
import :list;

export auto thread_start(char const* name,u8 prio,function func,void* func_arg) -> task*;

task* main_thread;  // 主线程 PCB
list thread_ready_list; // 线程就绪队列
list thread_all_list;   // 所有任务队列
list::node* thread_tag; // 用于保存队列中的线程结点

// 获取当前线程的 pcb 指针
auto running_thread() -> task*
{
    u32 esp;
    asm ("mov %%wsp, %0" : "=g"(esp));
    // 取整 得到PCB起始地址
    return reinterpret_cast<task*>(esp & 0xfffff000);
}

auto kernel_thread(function* func,void* func_arg) -> void
{
    intr_enable(); // 开中断，避免后面的时钟中断被屏蔽，无法调度其他线程
    func(func_arg);
}

auto thread_create(task* pthread,function func,void* func_arg) -> void
{
    // 内核栈 预留 中断栈与线程栈的空间
    pthread->self_kstack = reinterpret_cast<u32*>(reinterpret_cast<char*>(pthread->self_kstack) - sizeof(intr_stack) - sizeof(thread_stack));
    auto kthread_stack = reinterpret_cast<thread_stack*>(pthread->self_kstack);
    // *kthread_stack = (thread_stack) {
    //     .eip        =    kernel_thread,
    //     .func       =    func,
    //     .func_arg   =    func_arg,
    // };
    kthread_stack->eip = kernel_thread;
    kthread_stack->func = func;
    kthread_stack->func_arg = func_arg;
}

auto init_thread(task* pthread,char const* name,u8 prio) -> void
{
    // *pthread = (task) {
    //     .self_kstack    =      reinterpret_cast<u32*>(reinterpret_cast<u32>(pthread) + PG_SIZE),
    //                                             // 线程自己在内核态下，使用的栈顶地址
    //     .stu            =      status::running,
    //     .priority       =      prio,
    //     .name           =      {},
    //     .stack_magic    =      0x19870916  // 魔数
    // };
    if(pthread == main_thread) {
        // 将 main 函数封装为一个线程，main本就当前正在运行
        pthread->stu = status::running;
    } else {
        pthread->stu = status::ready;
    }
    pthread->self_kstack = reinterpret_cast<u32*>(reinterpret_cast<u32>(pthread) + PG_SIZE);
    pthread->priority = prio;
    pthread->ticks = prio;  // 优先级控制占用时长 大优先级占用大时长
    pthread->elapsed_ticks = 0;
    pthread->pgdir = nullptr;
    pthread->stack_magic = 0x1987016;
    strcpy(pthread->name,name);
}

auto thread_start(char const* name,u8 prio,function func,void* func_arg) -> task*
{
    auto thread = reinterpret_cast<task*>(get_kernel_pages(1)); // PCB 定义其 占用一页
    init_thread(thread,name,prio);
    thread_create(thread,func,func_arg);

    // 确保之前不在就绪队列中
    ASSERT(not thread_ready_list.contains(&thread->general_tag));
    // 加入就绪队列
    thread_ready_list.push_back(&thread->general_tag);

    ASSERT(not thread_all_list.contains(&thread->all_list_tag));
    thread_all_list.push_back(&thread->all_list_tag);

    // asm volatile (
    //     "mov %0, %%esp;"
    //     "pop %%ebp;"
    //     "pop %%ebx;"
    //     "pop %%edi;"
    //     "pop %%esi;"
    //     "ret;" // 栈顶地址为 kernel_thread
    //     :
    //     : "g"(thread->self_kstack)
    //     : "memory"
    // );

    return thread;
}

// 将 kernel 的执行流完善为主线程
auto make_main_thread() -> void
{
    // loader.asm 载入内核时 mov esp, 0xc009f000
    // 则 pcb 地址为 0xc009e000, 不需要额外分配一页
    main_thread = running_thread();
    init_thread(main_thread,"main",31);

    // main是当前运行的线程 当前线程当然不在就绪队列中，只加入all_list
    ASSERT(not thread_all_list.contains(&main_thread->all_list_tag));
    thread_all_list.push_back(&main_thread->all_list_tag);
}








