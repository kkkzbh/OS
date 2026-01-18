module;

#include <string.h>
#include <global.h>
#include <stdio.h>
#include <interrupt.h>
#include <assert.h>

export module thread:exec;

import :utility;
import :stack;
import alloc;
import task;
import schedule;
import algorithm;
import thread.pid;
import memory.utility;

task* main_thread;  // 主线程 PCB

auto kernel_thread(function* func,void* func_arg) -> void
{
    intr_enable(); // 开中断，避免后面的时钟中断被屏蔽，无法调度其他线程
    func(func_arg);
}

export auto thread_create(task* pthread,function func,void* func_arg) -> void
{
    // 内核栈 预留 中断栈与线程栈的空间
    pthread->self_kstack = reinterpret_cast<u32*>(reinterpret_cast<char*>(pthread->self_kstack) - sizeof(intr_stack) - sizeof(thread_stack));

    auto kthread_stack = reinterpret_cast<thread_stack*>(pthread->self_kstack);
    kthread_stack->eip = kernel_thread;
    kthread_stack->func = func;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

export auto init_thread(task* pthread,char const* name,u8 prio) -> void
{
    memset(pthread,0,sizeof(*pthread));
    pthread->pid = pid_pool.allocate();
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
    pthread->fd_table = { 0,1,2 }; // 0 1 2 分别为 标准输入 输出 错误
    pthread->fd_table[3,MAX_FILES_OPEN_PER_PROC] | std::fill[-1]; // 其余置 -1 NOLINT
    pthread->pgdir = nullptr;
    pthread->cwd_inode_no = 0;  // 任务的默认工作目录为根目录
    pthread->parent_pid = -1;   // 表示没有父进程
    pthread->stack_magic = 0x19870916;
    strcpy(pthread->name,name);
}

export auto thread_start(char const* name,u8 prio,function func,void* func_arg) -> task*
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

// 回收thread的pcb和页表，并从调度队列中移除
auto thread_exit(task* thread,bool need_schedule) -> void
{
    intr_disable(); // 确保关中断
    thread->stu = thread_status::died;
    // 如果该thread不是当前线程，则可能在就绪队列中，将其移除
    if(thread_ready_list.contains(&thread->general_tag)) {
        list::erase(&thread->general_tag);
    }
    if(thread->pgdir) {     // 如果是进程，回收进程的页表
        // TODO: 当前只回收了页目录表(1页)，二级页表资源回收不完整，存在内存泄漏:
        //   1. 需要遍历用户空间PDE(0-767)，释放所有有效的页表页
        //   2. 需要回收用户空间分配的所有物理页(代码、数据、栈等)
        //   3. 需要回收 userprog_vaddr.bits 虚拟地址位图
        mfree_page(pool_flags::KERNEL,thread->pgdir,1);
    }
    // 从all_thread_list中去掉该任务
    list::erase(&thread->all_list_tag);
    // 回收pcb所在的页，主线程的pcb不在堆中，跨国
    if(thread != main_thread) {
        mfree_page(pool_flags::KERNEL,thread,1);
    }
    // 回收pid
    pid_pool.release(thread->pid);
    // 如果需要下一轮调度则主动调度schedule
    if(need_schedule) {
        schedule();
        PANIC("thread_exit: should not be here!\n");
    }
}

export auto find_task_by(i32 pid) -> task*
{
    for(auto& node : thread_all_list) {
        auto* t = find_task_by_all(&node);
        if(t->pid == pid) {
            return t;
        }
    }
    return nullptr;
}

// 将 kernel 的执行流完善为主线程
export auto make_main_thread() -> void
{
    // loader.asm 载入内核时 mov esp, 0xc009f000
    // 则 pcb 地址为 0xc009e000, 不需要额外分配一页
    main_thread = running_thread();
    init_thread(main_thread,"main",31);

    // main是当前运行的线程 当前线程当然不在就绪队列中，只加入all_list
    ASSERT(not thread_all_list.contains(&main_thread->all_list_tag));
    thread_all_list.push_back(&main_thread->all_list_tag);
}






