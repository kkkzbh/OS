module;

#include <interrupt.h>
#include <global.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

export module process;

import thread;
import alloc;
import schedule;
import utility;
import task;
import memory;
import arena;

export auto process_execute(void* filename,char const* name) -> void;

auto constexpr USER_STACK3_VADDR = 0xc0000000 - 0x1000;

auto constexpr USER_VADDR_START  = 0x8048000;

auto constexpr default_prio      = 31;

// 构建用户进程上下文
auto start_process(void* filename) -> void
{
    auto func = filename;
    auto cur = running_thread();
    cur->self_kstack = (u32*)((char*)cur->self_kstack + sizeof(thread_stack));
    auto proc_stack = (intr_stack*)cur->self_kstack;
    *proc_stack = (intr_stack) {
        .fs     = SELECTOR_U_DATA,
        .es     = SELECTOR_U_DATA,
        .ds     = SELECTOR_U_DATA,
        .eip    = (void(*)())func,
        .cs     = SELECTOR_U_CODE,
        .eflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1,
        .esp    = (void*)((u32)get_a_page(pool_flags::USER,USER_STACK3_VADDR) + PG_SIZE),
        .ss     = SELECTOR_U_DATA
    };

    asm volatile("movl %0, %%esp; jmp intr_exit" : : "g"(proc_stack) : "memory");
}

// 创建用户进程虚拟地址位图
auto create_user_vaddr_bitmap(task* user_prog) -> void
{
    user_prog->userprog_vaddr.init (
        (char*)get_kernel_pages(
            std::div_ceil((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8,PG_SIZE)
        ),
        ((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8),
        USER_VADDR_START
    );
}

// 创建用户进程
auto process_execute(void* filename,char const* name) -> void
{
    // pcb内核数据结构，由内核维护进程信息，故在内核内存池中申请
    auto thread = (task*)get_kernel_pages(1);
    init_thread(thread,name,default_prio);
    create_user_vaddr_bitmap(thread);
    thread_create(thread,start_process,filename);
    thread->pgdir = create_page_dir();
    block_desc_init(thread->u_block_desc);
    auto old_status = intr_disable();
    ASSERT(not thread_ready_list.contains(&thread->general_tag));
    thread_ready_list.push_back(&thread->general_tag);

    ASSERT(not thread_all_list.contains(&thread->all_list_tag));
    thread_all_list.push_back(&thread->all_list_tag);
    intr_set_status(old_status);
}


