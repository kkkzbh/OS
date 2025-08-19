

export module sys;

import syscall.utility;

namespace std
{
    export template<typename... Args>
    requires (sizeof... (Args) <= 3)
    auto syscall(int num,Args... args) -> int
    {
        int ret;
        if constexpr (auto constexpr cnt = sizeof... (Args); cnt == 0) {
            asm volatile (
                "int $0x80"
                : "=a"(ret)
                : "a"(num)
                : "memory"
            );
        } else if constexpr (cnt == 1) {
            asm volatile (
                "int $0x80"
                : "=a"(ret)
                : "a"(num),"b"(args...[0])
                : "memory"
            );
        } else if constexpr (cnt == 2) {
            asm volatile (
                "int $0x80"
                : "=a"(ret)
                : "a"(num),"b"(args...[0]),"c"(args...[1])
                : "memory"
            );
        } else if constexpr (cnt == 3) {
            asm volatile (
                "int $0x80"
                : "=a"(ret)
                : "a"(num),"b"(args...[0]),"c"(args...[1]),"d"(args...[2])
                : "memory"
            );
        }

        return ret;
    }

    export auto getpid() -> u32
    {
        return syscall(+sysid::getpid);
    }

    export auto write(char const* str) -> u32
    {
        return syscall(+sysid::write,str);
    }

    export auto malloc(size_t sz) -> void*
    {
        return (void*)syscall(+sysid::malloc,sz);
    }

    export auto free(void* ptr) -> void
    {
        syscall(+sysid::free,ptr);
    }

}