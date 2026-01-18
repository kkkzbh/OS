module;

#include <stdio.h>

export module sys;

import syscall.utility;
import string;
import filesystem.utility;
import dir.structure;
import stat.structure;

using namespace fs;

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

    export auto write(i32 fd,void const* buf,u32 count) -> i32
    {
        return syscall(+sysid::write,fd,buf,count);
    }

    export auto malloc(size_t sz) -> void*
    {
        return (void*)syscall(+sysid::malloc,sz);
    }

    export auto free(void* ptr) -> void
    {
        syscall(+sysid::free,ptr);
    }

    export auto fork() -> pid_t
    {
        return syscall(+sysid::fork);
    }

    export auto read(i32 fd,void* buf,u32 count) -> i32
    {
        return syscall(+sysid::read,fd,buf,count);
    }

    export auto clear() -> void
    {
        syscall(+sysid::clear);
    }

    export auto putchar(int c) -> void
    {
        syscall(+sysid::putchar,c);
    }

    export auto getcwd(char* buf,size_t size) -> char*
    {
        return (char*)syscall(+sysid::getcwd,buf,size);
    }

    export auto open(std::string_view<char const> pathname,open_flags flags) -> i32
    {
        return syscall(+sysid::open,pathname.data(),+flags);
    }

    export auto close(i32 fdi) -> bool
    {
        return syscall(+sysid::close,fdi);
    }

    export auto lseek(i32 fd,i32 offset,whence flag) -> i32
    {
        return syscall(+sysid::lseek,fd,offset,flag);
    }

    export auto unlink(std::string_view<char const> pathname) -> bool
    {
        return syscall(+sysid::unlink,pathname.data());
    }

    export auto mkdir(std::string_view<char const> pathname) -> bool
    {
        return syscall(+sysid::mkdir,pathname.data());
    }

    export auto opendir(std::string_view<char const> pathname) -> dir*
    {
        return (dir*)syscall(+sysid::opendir,pathname.data());
    }

    export auto closedir(dir* dir) -> bool
    {
        return syscall(+sysid::closedir,dir);
    }

    export auto chdir(std::string_view<char const> path) -> bool
    {
        return syscall(+sysid::chdir,path.data());
    }

    export auto rmdir(std::string_view<char const> pathname) -> bool
    {
        return syscall(+sysid::rmdir,pathname.data());
    }

    export auto readdir(dir* dir) -> dir_entry*
    {
        return (dir_entry*)syscall(+sysid::readdir,dir);
    }

    export auto rewinddir(dir* dir) -> void
    {
        syscall(+sysid::rewinddir,dir);
    }

    export auto stat(std::string_view<char const> path,stat_t* buf) -> bool
    {
        return syscall(+sysid::stat,path.data(),buf);
    }

    export auto ps() -> void
    {
        syscall(+sysid::ps);
    }

    export auto exec(char const* path,char* argv[]) -> i32
    {
        return syscall(+sysid::exec,path,argv);
    }

    export auto wait(i32& status) -> pid_t
    {
        return syscall(+sysid::wait,status);
    }

    export [[noreturn]] auto exit(i32 status) -> void
    {
        syscall(+sysid::exit,status);
        __builtin_unreachable();
    }

}