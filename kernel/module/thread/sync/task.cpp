module;

#include <assert.h>

export module task;

import utility;
import memory;
import bitmap;
import list;
import arena;
import array;

struct virtual_addr : bitmap
{
    auto init(char* addr,size_t sz,u32 vstart) -> void
    {
        bits = (u8*)addr;
        this->sz = sz;
        vaddr_start = vstart;
    }

    // 在虚拟内存池中申请 pg_cnt 个虚拟页
    // 成功返回虚拟起始地址 失败返回 nullptr
    auto get(u32 pg_cnt) -> void*
    {
        auto start = scan(pg_cnt);
        if(not start) {
            return nullptr;
        }
        set(*start,*start + pg_cnt,true);
        auto vaddr = vaddr_start + *start * PG_SIZE;
        return reinterpret_cast<void*>(vaddr);
    }

    using bitmap::set;

    // 将虚拟地址 转换为对应的位的位置
    auto trans(u32 vaddr) const -> size_t
    {
        ASSERT(vaddr >= vaddr_start);
        return (vaddr - vaddr_start) / PG_SIZE;
    }

    // 将 vaddr对应的位图位 置1
    auto set(u32 vaddr) -> void
    {
        set(trans(vaddr),true);
    }

    u32 vaddr_start;
};

export auto constexpr MAX_FILES_OPEN_PER_PROC = 8; // 每个进程允许打开的文件数

export struct task
{
    u32* self_kstack;       // 各个内核线程都用自己的内核栈
    pid_t pid;
    thread_status stu;             // 状态
    char name[16];
    u8 priority;            // 优先级
    u8 ticks;               // 每次在处理器上执行的时间嘀嗒数

    u32 elapsed_ticks;      // 此任务上cpu运行后至今占用了多少cpu嘀嗒数

    std::array<i32,MAX_FILES_OPEN_PER_PROC> fd_table;  // 文件描述符数组

    list::node general_tag; // 用于线程在一般的队列中的节点
    list::node all_list_tag;// 用于线程队列thread_all_list中的节点

    u32* pgdir;             // 进程自己页目录表的虚拟地址
    virtual_addr userprog_vaddr;    // 用户进程的虚拟地址
    mem_block_desc u_block_desc[DESC_CNT];
    u32 cwd_inode_no;       // 进程所在的工作目录的inode编号
    i16 parent_pid;         // 父进程的pid
    u32 stack_magic;        // 栈的边界标记，防止溢出
};

export auto find_task_by_general(list::node* tag) -> task*;

export auto find_task_by_all(list::node* tag) -> task*;

auto find_task_by_general(list::node* tag) -> task*
{
    return reinterpret_cast<task*>(
        reinterpret_cast<u32>(tag) - reinterpret_cast<u32>(&reinterpret_cast<task*>(0)->general_tag)
    );
}

auto find_task_by_all(list::node* tag) -> task*
{
    return reinterpret_cast<task*>(
        reinterpret_cast<u32>(tag) - reinterpret_cast<u32>(&reinterpret_cast<task*>(0)->all_list_tag)
    );
}