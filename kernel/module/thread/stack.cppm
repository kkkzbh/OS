

module thread:stack;

import :utility;

import :list;

// 中断栈数据结构
// 维护中断发生时，保护程序(线程或进程)的上下文环境
// 在线程自己的内核栈中位置固定，所在页的最顶端

struct intr_stack
{
    u32 vec_no;     // 中断向量号
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 esp_dummy; // pushad会压入esp，但popad会忽略esp
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;
    u32 gs;
    u32 fs;
    u32 es;
    u32 ds;

    // 当从低特权级陷入高特权级时压入
    u32 err_code;   // err_code 被压入在 eip 之后
    void (*eip)();
    u32 cs;
    u32 eflags;
    void* esp;
    u32 ss;
};

struct thread_stack
{
    u32 ebp;        // 这四个寄存器归主调函数所有，主调函数为调度器函数，要为他保存上下文环境
    u32 ebx;
    u32 edi;
    u32 esi;

    // 线程第一次执行时，eip指向待调用的函数，其他时候，eip指向 switch_to 的返回地址
    void (*eip) (function* func,void* func_arg);

    // 以下仅在第一次被调度上cpu时会使用
    void (*unused_retaddr);     // 占位 是返回地址在栈中的位置
    function* func;             // 内核线程所调用的函数
    void* func_arg;             // 内核线程调用函数传的参数
};


struct task
{
    u32* self_kstack;       // 各个内核线程都用自己的内核栈
    status stu;             // 状态
    char name[16];
    u8 priority;            // 优先级
    u8 ticks;               // 每次在处理器上执行的时间嘀嗒数

    u32 elapsed_ticks;      // 此任务上cpu运行后至今占用了多少cpu嘀嗒数

    list::node general_tag; // 用于线程在一般的队列中的节点
    list::node all_list_tag;// 用于线程队列thread_all_list中的节点

    u32* pgdir;             // 进程自己页表的虚拟地址
    u32 stack_magic;        // 栈的边界标记，防止溢出
};

