

export module bit;

import utility;

export namespace std
{

    // 将dst中cnt个相邻字节交换位置后存入buf
    // 1 2 3 4 5 -> 2 1 4 3 '\0'
    // 1 2 3 4   -> 2 1 4 3 '\0'
    auto pair_byte_swap(char const* dst,char* buf,u32 cnt) -> void
    {
        auto i = 0u;
        for(; i < cnt; i += 2) {
            buf[i + 1] = *dst++;
            buf[i] = *dst++;
        }
        buf[i] = '\0';
    }
}