module;

#include <assert.h>

export module exit;

import utility;
import schedule;
import wait_exit;
import thread;

// 子进程结束自己时的调用
export auto exit(i32 status) -> void
{
    auto child_thread = running_thread();
    child_thread->exit_status = status;
    if(child_thread->parent_pid == -1) {
        PANIC("sys_exit: child_thread->parent_pid is -1!\n");
    }
    // 将进程child_thread的所有子进程过继给init
    for(auto& nd : thread_all_list) {
        init_adopt_a_child(&nd,child_thread->pid);
    }
    // 回收进程child_thread的资源
    release_program_resource(child_thread);
    // 如果父进程正在等待子进程退出，则唤醒父进程
    auto parent_thread = find_task_by(child_thread->parent_pid);
    if(parent_thread->stu == thread_status::waiting) {
        thread_unblock(parent_thread);
    }
    // 将自己挂起，等待父进程获取自己的status，并回收pcb
    thread_block(thread_status::hanging);
}