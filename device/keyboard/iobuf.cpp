module;

#include <stdio.h>

export module iobuf;

import ioqueue;

export auto ioqbuf = ioqueue{};

extern "C" auto solve(char c) -> void
{
    if(not ioqbuf.full()) { // 如果键盘缓冲区没满
        // putchar(c);  // 不回显，仅测试中断用，应该由shell来控制是否回显
        ioqbuf.put(c);
    }
}