

module thread:utility;

export import utility;

// 维护进程与线程的状态
enum struct status
{
    running,
    ready,
    blocked,
    waiting,
    hanging,
    died
};

using function = void(void*);

