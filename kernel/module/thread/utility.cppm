

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

auto constexpr PG_SIZE = 4096;

using function = void(void*);

