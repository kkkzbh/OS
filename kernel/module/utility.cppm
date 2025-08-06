

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


template<typename T>
auto max(T const& x,T const& y) -> T const&
{
    if(y > x) {
        return y;
    }
    return x;
}

