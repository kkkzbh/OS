module;

#include <interrupt.h>
#include <string.h>
#include <assert.h>

export module fork;

import task;
import alloc;
import arena;
import utility;
import process;
import algorithm;
import schedule;
import file.manager;
import scope;
import memory;
import thread;
import thread.pid;

// 将父进程的pcb拷贝给子进程
auto copy_pcb_vaddr_bitmap_stack0(task* child_thread,task* parent_thread) -> void
{
    // 复制pcb所在的整个页，里面包含进程pcb信息以及特权0级的栈
    memcpy(child_thread,parent_thread,PG_SIZE);
    // 下面单独修改
    child_thread->pid = pid_pool.allocate();
    child_thread->elapsed_ticks = 0;
    child_thread->stu = thread_status::ready;
    child_thread->ticks = child_thread->priority;   // 为新进程把时间片充满
    child_thread->parent_pid = parent_thread->pid;
    child_thread->general_tag.prev = child_thread->general_tag.next = nullptr;
    child_thread->all_list_tag.prev = child_thread->all_list_tag.next = nullptr;
    block_desc_init(child_thread->u_block_desc);
    // 复制父进程的虚拟地址池的位图
    auto bitmap_pg_cnt = std::div_ceil((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8,PG_SIZE);
    auto vaddr_bitmap = get_kernel_pages(bitmap_pg_cnt);
    // 此时child_thread->userprog_vaddr.bits还是指向的父进程虚拟地址的位图地址
    // 下面让其指向自己的
    memcpy(vaddr_bitmap,child_thread->userprog_vaddr.bits,bitmap_pg_cnt * PG_SIZE);
    child_thread->userprog_vaddr.bits = (u8*)vaddr_bitmap;

    // 调试用
    ASSERT(strlen(child_thread->name) < 11);    // 避免下面的strcat越界，因为thread.name是char[16]

    strcat(child_thread->name,"_fork");
}

// 复制子进程的进程体(代码和数据) 以及用户栈
auto copy_body_stack3(task* child_thread,task* parent_thread,void* buf_page) -> void
{
    auto vaddr_bitmap = (u8*)(parent_thread->userprog_vaddr.bits);
    auto bitmap_bytes_len = parent_thread->userprog_vaddr.sz;
    auto vaddr_start = parent_thread->userprog_vaddr.vaddr_start;

    // 在父进程中的用户空间查找已有数据的页
    for(auto idx_byte : std::iota[bitmap_bytes_len]) {
        if(not vaddr_bitmap[idx_byte]) {
            continue;
        }
        for(auto idx_bit : std::iota[8]) {
            if(not (1 << idx_bit & vaddr_bitmap[idx_byte])) {
                continue;
            }
            auto prog_vaddr = (idx_byte * 8 + idx_bit) * PG_SIZE + vaddr_start;
            // 将父进程用户空间中的数据通过内核空间做中转，最终复制到子进程的用户空间
            // 1. 将父进程在用户空间中的数据复制到内核缓冲区buf_page
            // 目的是下面切换到子进程的页表后，还能访问到父进程的数据
            memcpy(buf_page,(void*)prog_vaddr,PG_SIZE);
            // 2. 将页表切换到子进程，目的是避免下面申请内存的函数，将pte及pde安装在父进程的页表中
            page_dir_active(child_thread);
            // 3. 申请虚拟地址prog_vaddr
            get_a_page_without_op_vaddr_bitmap(pool_flags::USER,prog_vaddr);
            // 4. 从内核缓冲区将父进程数据复制到子进程的用户空间
            memcpy((void*)prog_vaddr,buf_page,PG_SIZE);
            // 5. 恢复父进程页表
            page_dir_active(parent_thread);
        }
    }

}

// 为子进程构建thread_stack和修改返回值
auto build_child_stack(task* child_thread) -> void
{
    // 1. 使子进程pid返回值为0
    // 获取子进程0级栈栈顶
    auto intr_0_stack = (intr_stack*)((u32)child_thread + PG_SIZE - sizeof(intr_stack));
    // 修改子进程的返回值为0
    intr_0_stack->eax = 0;

    // 2. 为switch_to构建thread_stack(注意不要局限于stack中定义的那个struct)，将其构建在紧邻intr_stack之下的空间
    auto ret_addr_in_thread_stack = (u32*)intr_0_stack - 1;
    // 这三个只是为了表达线程栈的结构，实际并不需要
    auto esi_ptr_in_thread_stack = (u32*)intr_0_stack - 2;
    auto edi_ptr_in_threadd_stack = (u32*)intr_0_stack - 3;
    auto ebx_ptr_in_thread_stack = (u32*)intr_0_stack - 4;

    // ebp在thread_stack中的地址便是当时的esp(0级栈的栈顶)
    auto ebp_ptr_in_thread_stack = (u32*)intr_0_stack - 5;

    // switch_to的返回地址更新为intr_exit，直接从中断返回
    *ret_addr_in_thread_stack = (u32)intr_exit;
    // 理由同，实际并不需要，因为在进入intr_exit时，一系列上下文寄存器的pop会覆盖这些值
    *esi_ptr_in_thread_stack = *edi_ptr_in_threadd_stack = *ebp_ptr_in_thread_stack = 0;

    // 把构建的thread_stack的栈顶作为swith_to恢复数据时候的栈顶
    child_thread->self_kstack = ebp_ptr_in_thread_stack;
}

// 更新inode打开数
auto update_inode_cnts(task* thread) -> void
{
    for(auto local_fd : std::iota[3,MAX_FILES_OPEN_PER_PROC]) {
        auto global_fd = thread->fd_table[local_fd];
        ASSERT(global_fd < MAX_FILE_OPEN);
        if(global_fd == -1) {
            continue;
        }
        ++file_table[global_fd].node->open_cnts;
    }
}

// 拷贝父进程本身所占资源给子进程
auto copy_process(task* child_thread,task* parent_thread) -> bool
{
    // 思路仍然是利用内核缓冲区作为中转，使父进程用户空间的数据复制到子进程用户空间
    auto buf_page = get_kernel_pages(1);
    if(not buf_page) {
        return false;
    }
    auto constexpr active = true;
    auto free_buf_page = scope_exit {
        [&] {
            mfree_page(pool_flags::KERNEL,buf_page,1);
        },
        active
    };
    // 1. 复制父进程的pcb，虚拟地址位图，内核栈到子进程
    copy_pcb_vaddr_bitmap_stack0(child_thread,parent_thread);

    // 2.为子进程创建页表，此页表仅包含内核空间
    child_thread->pgdir = create_page_dir();
    if(not child_thread->pgdir) {
        return false;
    }

    // 3. 复制父进程进程体及用户栈给子进程
    copy_body_stack3(child_thread,parent_thread,buf_page);

    // 4. 构建子进程thread_stack和修改返回值pid
    build_child_stack(child_thread);

    // 5. 更新文件inode的打开数
    update_inode_cnts(child_thread);

    return true;
}

// fork子进程，内核线程不可直接调用
export auto fork() -> pid_t
{
    auto parent_thread = running_thread();
    auto child_thread = (task*)get_kernel_pages(1);
    // 为子进程创建pcb (task结构)
    if(not child_thread) {
        return -1;
    }
    ASSERT(intr_get_status() == intr_status::INTR_OFF and parent_thread->pgdir);
    if(not copy_process(child_thread,parent_thread)) {
        return -1;
    }
    // 添加到就绪线程队列和所有线程队列，子进程由调试器安排运行
    ASSERT(not thread_ready_list.contains(&child_thread->general_tag));
    thread_ready_list.push_back(&child_thread->general_tag);
    ASSERT(not thread_all_list.contains(&child_thread->all_list_tag));
    thread_all_list.push_back(&child_thread->all_list_tag);

    return child_thread->pid;   // 父进程返回子进程的pid
}