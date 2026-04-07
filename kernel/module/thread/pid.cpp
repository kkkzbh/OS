
export module thread.pid;

import utility;
import array;
import bitmap;
import mutex;
import lock_guard;

struct pid_pool : bitmap
{
    auto init() -> void
    {
        bits = pid_bitmap.data();
        sz = pid_bitmap.size();
        for(auto& byte : pid_bitmap) {
            byte = 0;
        }
        mtx.init();
    }

    auto allocate() -> pid_t
    {
        assert_initialized();
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
        assert_initialized();
        auto lcg = lock_guard{ mtx };
        auto bi = pid - start;
        set(bi,false);
    }

    auto assert_initialized() const -> void
    {
        if(not (bits == pid_bitmap.data() and sz == pid_bitmap.size() and mtx.initialized())) [[unlikely]] {
            __builtin_trap();
        }
    }

    u32 constexpr static start = 1;
    std::array<u8,128> pid_bitmap;
    mutex mtx;
};

export pid_pool pid_pool;
