module;

#include <interrupt.h>
#include <string.h>

export module exec;

import utility;
import algorithm;
import memory;
import alloc;
import filesystem.syscall;
import filesystem.utility;
import scope;
import schedule;
import task;
import thread;
import console;

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

// 将文件描述符fd指向的文件中，偏移为offset，大小为filesz的段加载到虚拟地址vaddr的内存
auto segment_load(i32 fd,u32 offset,u32 filesz,u32 vaddr) -> bool
{
    auto vaddr_first_page = vaddr & 0xfffff000;  // 虚拟地址所在的页框
    auto size_in_first_page = PG_SIZE - (vaddr & 0x00000fff);   // 加载到内存后，文件在第一个页框中可以占用的字节大小
    auto occupy_pages = 0u;
    if(filesz > size_in_first_page) {   // 如果第一个页框容不下该段
        auto left_size = filesz - size_in_first_page;
        occupy_pages = std::div_ceil(left_size,PG_SIZE) + 1;    // +1 是指第一张页
    } else {
        occupy_pages = 1;
    }
    // 为进程分配内存
    auto vaddr_page = vaddr_first_page;
    for(auto i : std::iota[occupy_pages]) {
        auto pde = pgtable::pde_ptr(vaddr_page);
        auto pte = pgtable::pte_ptr(vaddr_page);
        // 如果pde或者pte不在，需要分配内存，pde的判断要在pte之前，否则pde不存在会导致判断pte时缺页异常
        if(not (*pde & 0x00000001) or not(*pte & 0x00000001)) {
            if(not get_a_page(pool_flags::USER,vaddr_page)) {
                return false;
            }
        }
        // 如果原进程的页表已经分配了，直接利用现有的物理页，直接覆盖进程体
        vaddr_page += PG_SIZE;
    }
    lseek(fd,offset,whence::set);
    read(fd,(void*)vaddr,filesz);
    return true;
}

// 从文件系统上加载用户程序pathname，成功则返回程序的起始地址，失败返回(false nullptr 0)
auto load(char const* pathname) -> i32
{
    auto elf_header = elf32::ehdr{};
    auto prog_header = elf32::phdr{};
    auto fd = open(pathname,+open_flags::read);
    if(fd == -1) {
        console::println("can not open {}!",pathname);
        return 0;
    }
    auto constexpr active = true;
    auto close_fd = scope_exit {
        [&] {
            close(fd);
        },
        active
    };
    if(read(fd,&elf_header,sizeof elf_header) != sizeof elf_header) {
        console::println("read {} elf_header failed!",pathname);
        return 0;
    }

    // // 调试信息：打印ELF头原始数据
    // console::println("=== ELF Header Debug Info ===");
    // console::println("ELF Magic: {:02x} {:02x} {:02x} {:02x}",
    //     (u8)elf_header.ident[0], (u8)elf_header.ident[1], (u8)elf_header.ident[2], (u8)elf_header.ident[3]);
    // console::println("Class: {}, Data: {}, Version: {}",
    //     elf_header.ident[4], elf_header.ident[5], elf_header.ident[6]);
    // console::println("Type: {}, Machine: {}, Version: {}",
    //     elf_header.type, elf_header.machine, elf_header.version);
    // console::println("Entry: {:#x}, Phoff: {}, Phentsize: {}, Phnum: {}",
    //     elf_header.entry, elf_header.phoff, elf_header.phentsize, elf_header.phnum);
    // console::println("==============================");

    // 检验elf头
    if (
        memcmp(elf_header.ident,"\177ELF\1\1\1",7)
        or elf_header.type != 2
        or elf_header.machine != 3
        or elf_header.version != 1
        or elf_header.phnum > 1024
        or elf_header.phentsize != sizeof(elf32::phdr)
    ) {
        console::println("{} elf_header is not correct!",pathname);
        if(memcmp(elf_header.ident,"\177ELF\1\1\1",7)) {
            console::println("elf_header ident not correct! Expected: \\177ELF\\1\\1\\1");
        } else if(elf_header.type != 2) {
            console::println("elf_header type not correct! Expected: 2 (ET_EXEC), Got: {}", elf_header.type);
        } else if(elf_header.machine != 3) {
            console::println("elf_header machine not correct! Expected: 3 (EM_386), Got: {}", elf_header.machine);
        } else if(elf_header.version != 1) {
            console::println("elf_header version not correct! Expected: 1, Got: {}", elf_header.version);
        } else if(elf_header.phnum > 1024) {
            console::println("elf_header phnum too large! Expected: <= 1024, Got: {}", elf_header.phnum);
        } else if(elf_header.phentsize != sizeof(elf32::phdr)) {
            console::println("elf_header phentsize not correct! Expected: {}, Got: {}", sizeof(elf32::phdr), elf_header.phentsize);
        }
        return 0;
    }
    auto prog_header_offset = elf_header.phoff;
    auto prog_header_size = elf_header.phentsize;

    for(auto prog_idx : std::iota[elf_header.phnum]) {  // 遍历所有程序头
        prog_header = {};
        lseek(fd,prog_header_offset,whence::set);   // 将文件的指针定位到程序头
        if(read(fd,&prog_header,prog_header_size) != prog_header_size) {
            return 0;
        }
        if(prog_header.type == +elf32::segment::load) { // 如果是可加载段就调用segment_load加载到内存
            if(not segment_load(fd,prog_header.offset,prog_header.filesz,prog_header.vaddr)) {
                return 0;
            }
        }
        prog_header_offset += elf_header.phentsize; // 更新下一个程序头的偏移
    }
    return elf_header.entry;    // 返回入口(程序起始地址)
}

// 用path指向的程序替换当前进程
export auto exec(char const* path,char* argv[]) -> i32
{
    auto argc = 0u;
    while(argv[argc]) {
        ++argc;
    }
    auto entry_point = load(path);
    if(not entry_point) {   // 如果加载失败
        console::println("load entry_point failed! cannot load {}",path);
        return false;
    }
    auto cur = running_thread();
    auto constexpr task_name_size = sizeof(task{}.name);
    memcpy(cur->name,path,task_name_size);
    cur->name[task_name_size - 1] = '\0';   // 空终止
    auto intr_0_stack = (intr_stack*)((u32)cur + PG_SIZE - sizeof(intr_stack));
    // 参数传递给用户进程
    intr_0_stack->ebx = (i32)argv;
    intr_0_stack->ecx = argc;
    intr_0_stack->eip = (void(*)())entry_point;
    intr_0_stack->esp = (void*)0xc0000000;  // 新用户进程的栈地址为最高用户空间地址
    asm volatile (      // exec不同于fork，为使新进程更快被执行，直接从中断返回
        "movl %0, %%esp;"
        "jmp intr_exit;"
        :
        : "g"(intr_0_stack)
        : "memory"
    );
    return true;    // 理论上没有返回值了
}