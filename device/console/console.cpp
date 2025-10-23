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

    auto constexpr BUFSZ = 1024;

    char buf[BUFSZ]{};

    template<typename... Args>
    auto print(char const* format,Args&&... args) -> u32
    {
        auto ret = std::format_to(buf,format,std::forward<Args>(args)...);
        buf[ret] = '\0';
        write(buf);
        return ret;
    }

    auto println() -> u32
    {
        putc('\n');
        return 1;
    }

    template<typename... Args>
    auto println(char const* format,Args&&... args) -> u32
    {
        auto end = std::format_to(buf,format,std::forward<Args>(args)...);
        buf[end++] = '\n';
        buf[end] = '\0';
        write(buf);
        return end;
    }

}

namespace console
{

    auto write(char const* str) -> void
    {
        // auto lcg = lock_guard{ mtx };
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


