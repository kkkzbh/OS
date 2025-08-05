module;

#include <stdio.h>

export module iobuf;

import ioqueue;

auto buf = ioqueue{};

extern "C" auto solve(char c) -> void
{
    if(not buf.full()) { // 如果键盘缓冲区没满
        putchar(c);
        buf.put(c);
    }
}