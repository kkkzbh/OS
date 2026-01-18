

export module thread.pid;

import utility;
import array;
import bitmap;
import mutex;
import lock_guard;

struct pid_pool : bitmap
{
    pid_pool() : bitmap{ pid_bitmap.data(),pid_bitmap.size() }
    {}

    auto allocate() -> pid_t
    {
        auto lcg = lock_guard{ mtx };
        auto bi = scan(1);
        if(not bi) {
            return -1;
        }
        set(*bi,true);
        return start + *bi;
    }

    auto release(pid_t const pid) -> void
    {
        auto lcg = lock_guard{ mtx };
        auto bi = pid - start;
        set(bi,false);
    }

    u32 constexpr static start = 1;
    std::array<u8,128> pid_bitmap{};
    mutex mtx{};
};

export pid_pool pid_pool{};