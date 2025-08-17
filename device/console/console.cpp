module;

#include <stdio.h>

export module console;

import utility;
import mutex;
import lock_guard;

export namespace console
{
    auto write(char const* str) -> void;

    auto writeline(char const* str) -> void;

    auto putc(int c) -> void;

    auto puth(u32 v) -> void;
}

namespace console
{
    auto mtx = mutex{};

    auto write(char const* str) -> void
    {
        auto lcg = lock_guard{ mtx };
        puts(str);
    }

    auto writeline(char const* str) -> void
    {
        auto lcg = lock_guard{ mtx };
        puts(str);
        putc('\n');
    }

    auto putc(int c) -> void
    {
        auto lcg = lock_guard{ mtx };
        putchar(c);
    }

    auto puth(u32 v) -> void
    {
        auto lcg = lock_guard{ mtx };
        puthex(v);
    }

}


