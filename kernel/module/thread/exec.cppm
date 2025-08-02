module;

#include <string.h>
#include <global.h>
#include <stdio.h>

export module thread:exec;

import :utility;
import memory;
import :stack;

export auto thread_start(char const* name,u8 prio,function func,void* func_arg) -> task*;

auto kernel_thread(function* func,void* func_arg) -> void
{
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
    pthread->self_kstack = reinterpret_cast<u32*>(reinterpret_cast<u32>(pthread) + PG_SIZE);
    pthread->stu = status::running;
    pthread->priority = prio;
    pthread->stack_magic = 0x1987016;
    strcpy(pthread->name,name);
}

auto thread_start(char const* name,u8 prio,function func,void* func_arg) -> task*
{
    auto thread = reinterpret_cast<task*>(get_kernel_pages(1)); // PCB 定义其 占用一页
    puts("the thread addr is: ");
    thread->self_kstack = reinterpret_cast<u32*>(0x12345);
    puthex(reinterpret_cast<u32>(thread));
    putchar('\n');
    init_thread(thread,name,prio);
    thread_create(thread,func,func_arg);

    asm volatile (
        "mov %0, %%esp;"
        "pop %%ebp;"
        "pop %%ebx;"
        "pop %%edi;"
        "pop %%esi;"
        "ret;" // 栈顶地址为 kernel_thread
        :
        : "g"(thread->self_kstack)
        : "memory"
    );
    return thread;
}











