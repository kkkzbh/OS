

export module sync:task;

import utility;
import :list;
import memory;

export struct task
{
    u32* self_kstack;       // 各个内核线程都用自己的内核栈
    thread_status stu;             // 状态
    char name[16];
    u8 priority;            // 优先级
    u8 ticks;               // 每次在处理器上执行的时间嘀嗒数

    u32 elapsed_ticks;      // 此任务上cpu运行后至今占用了多少cpu嘀嗒数

    thread_list::node general_tag; // 用于线程在一般的队列中的节点
    thread_list::node all_list_tag;// 用于线程队列thread_all_list中的节点

    u32* pgdir;             // 进程自己页目录表的虚拟地址
    virtual_addr userprog_vaddr;    // 用户进程的虚拟地址
    u32 stack_magic;        // 栈的边界标记，防止溢出
};

export auto find_task_by_general(thread_list::node* tag) -> task*;

export auto find_task_by_all(thread_list::node* tag) -> task*;

auto find_task_by_general(thread_list::node* tag) -> task*
{
    return reinterpret_cast<task*>(
        reinterpret_cast<u32>(tag) - reinterpret_cast<u32>(&reinterpret_cast<task*>(0)->general_tag)
    );
}

auto find_task_by_all(thread_list::node* tag) -> task*
{
    return reinterpret_cast<task*>(
        reinterpret_cast<u32>(tag) - reinterpret_cast<u32>(&reinterpret_cast<task*>(0)->all_list_tag)
    );
}