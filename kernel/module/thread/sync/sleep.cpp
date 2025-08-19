

export module sleep;

import utility;
import schedule;

extern u32 ticks;  // 内核自中断开启以来总共的嘀嗒数

auto constexpr IRQ0_FREQ = u32(100);

auto constexpr mseconds_perintr = (1000 / IRQ0_FREQ);

export auto operator""ms(u32 v) -> u32  // NOLINT
{
    auto tik = std::div_ceil(v,mseconds_perintr);
    return tik;
}

export auto operator""s(u32 v) -> u32   // NOLINT
{
    return operator""ms(v * 1000);
}

export auto sleep(u32 tik) -> void
{
    auto start = tik;
    while(ticks - start < tik) {
        thread_yield();
    }
}