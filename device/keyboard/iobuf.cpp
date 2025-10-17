module;

#include <stdio.h>

export module iobuf;

import ioqueue;

export auto ioqbuf = ioqueue{};

extern "C" auto solve(char c) -> void
{
    if(not ioqbuf.full()) { // 如果键盘缓冲区没满
        putchar(c);
        ioqbuf.put(c);
    }
}