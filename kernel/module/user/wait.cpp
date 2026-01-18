

export module wait;

import wait_exit;
import utility;
import schedule;
import task;
import thread;
import list.node;

export auto wait(i32& status) -> pid_t
{
    auto parent_thread = running_thread();
    while(true) {
        auto find = [&](auto func) -> list_node* {
            for(auto& nd : thread_all_list) {
                if(func(&nd,parent_thread->pid)) {
                    return &nd;
                }
            }
            return nullptr;
        };
        // 优先处理已经挂起的任务
        auto cnd = find(find_hanging_child);
        if(cnd) {   //若存在已挂起的子进程
            auto child_thread = find_task_by_all(cnd);
            status = child_thread->exit_status;
            auto child_pid = child_thread->pid;
            // 从就绪队列和全部队列移除PCB并回收PCB
            thread_exit(child_thread,false);
            return child_pid;
        }
        // 判断是否存在子进程
        cnd = find(find_child);
        if(not cnd) {   // 如果不存在子进程，出错返回
            return -1;
        }
        // 若子进程还未运行完成，挂起自己，直到子进程执行exit唤醒自己
        thread_block(thread_status::waiting);
    }
}