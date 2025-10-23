

export module write_execution;

import utility;
import filesystem.utility;
import ide;
import vector;
import filesystem.syscall;
import console;
import sys;
import scope;

namespace elf32
{

    using word = u32;
    using addr = u32;
    using off = u32;
    using half = u16;

    // 32位elf头
    struct ehdr
    {
        char ident[16];
        half type;
        half machine;
        word version;
        addr entry;
        off  phoff;
        off  shoff;
        word falgs;
        half ehsize;
        half phentsize;
        half phnum;
        half shentsize;
        half shnum;
        half shstrndx;
    };

    // 程序头表 Program header 段描述头
    struct phdr
    {
        word type;
        off  offset;
        addr vaddr;
        addr paddr;
        word filesz;
        word memsz;
        word flags;
        word align;
    };

    enum struct segment
    {
        null,       // 忽略
        load,       // 可加载程序段
        dynamic,    // 动态加载信息
        interp,     // 动态加载器名称
        note,       // 一些辅助信息
        shlib,      // 保留
        phdr,       // 程序头表
    };

    auto operator+(segment seg) -> auto
    {
        return __underlying_type(segment)(seg);
    }

}

// 写入应用程序 std_print
auto std_print() -> void
{
    auto constexpr filesz = 51200;
    auto constexpr sec_cnt = std::div_ceil(filesz,SECTOR_SIZE);
    auto sda = &channels[0].devices[0];
    auto program_buf = std::vector(filesz,char{});

    ide_read(sda,1000,program_buf.data(),sec_cnt);

    auto constexpr target = "/bin/prog_no_arg";
    auto fd = open(target,+open_flags::create | +open_flags::rdwr);
    if(fd == -1) {
        console::println("can not open {}",target);
        return;
    }

    if(auto wsz = write(fd,program_buf.data(),filesz); wsz != filesz) {
        console::println("file {} write error! need {} bytes but {} bytes",target,filesz,wsz);
        while(true) {}
        return;
    }
    close(fd);
    console::println("write {}",target);

}

export auto write_execution() -> void
{
    // std_print();
}