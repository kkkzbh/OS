module;

#include <time.h>

export module sleep;

import utility;
import schedule;

auto constexpr IRQ0_FREQ = u32(100);

auto constexpr mseconds_perintr = (1000 / IRQ0_FREQ);

export auto operator""ms(unsigned long long v) -> u32  // NOLINT
{
    auto tik = std::div_ceil(u32(v),mseconds_perintr);
    return tik;
}

export auto operator""s(unsigned long long v) -> u32   // NOLINT
{
    return operator""ms(u32(v) * 1000);
}

export auto sleep(u32 tik) -> void
{
    auto start = ticks;  // 记录当前时间
    while(ticks - start < tik) {
        thread_yield();
    }
}