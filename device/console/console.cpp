module;

#include <stdio.h>
#include <string.h>

export module console;

import utility;
import mutex;
import lock_guard;
import format;

auto mtx = mutex{};

export namespace console
{
    auto write(char const* str) -> void;

    auto writeline(char const* str) -> void;

    auto putc(int c) -> void;

    auto puth(u32 v) -> void;

    template<typename... Args>
    auto print(char const* format,Args&&... args) -> u32
    {
        auto lcg = lock_guard{ mtx };
        char buf[1024]{};
        format_to(buf,format,args...);
        write(buf);
        return strlen(buf);
    }

    auto println() -> u32
    {
        putc('\n');
        return 1;
    }

    template<typename... Args>
    auto println(char const* format,Args&&... args) -> u32
    {
        auto lcg = lock_guard{ mtx };
        char buf[1024]{};
        format_to(buf,format,args...);
        write(buf);
        println();
        return strlen(buf);
    }

}

namespace console
{

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


