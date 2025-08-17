

export module utility;

export
{
    #include <stdint.h>
}

// 页表一页的大小 B
export auto constexpr PG_SIZE = 4096;

export enum struct thread_status
{
    running,
    ready,
    blocked,
    waiting,
    hanging,
    died
};

export using thread_function = void(void*);

export using pid_t = u16;

export template<typename T>
auto max(T const& x,T const& y) -> T const&
{
    if(y > x) {
        return y;
    }
    return x;
}

export template<typename T>
auto div_ceil(T x,auto p)
{
    return (x + p - 1) / p;
}