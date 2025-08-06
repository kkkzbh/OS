

export module memory:utility;

export import utility;

// 0xc009f000 是内核主线程栈顶，0xc009e000是内核主线程的pcb
// 一个页框大小的位图可表示 128MB 内存，位图位置安排在 0xc009a000
// 则支持 4 个页框的位图 共 512MB
auto constexpr MEM_BITMAP_BASE = 0xc009a000;

// 跨过内核虚拟地址的低端1MB
auto constexpr K_HEAP_START = 0xc0100000;

export enum struct pool_flags
{
    KERNEL,
    USER
};

auto constexpr PG_P_1   =   1;  // 页表项或页目录项 存在 属性位
auto constexpr PG_P_0   =   0;

auto constexpr PG_RW_R  =   0;  // R/W属性位，读/执行
auto constexpr PG_RW_W  =   2;  // 读/写/执行

auto constexpr PG_US_S  =   0;  // U/S 属性位值，系统级
auto constexpr PG_US_U  =   4;  // 用户级