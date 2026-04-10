module;

#include <string.h>

export module write_execution;

import utility;
import filesystem.utility;
import block.device;
import array;
import vector;
import filesystem.syscall;
import console;
import sys;

using namespace fs;

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

namespace
{
    struct staged_program
    {
        char const* source_device_name;
        u32 start_lba;
        u32 reserved_sectors;
        char const* target_path;
        bool required;
    };

    auto constexpr staged_programs = std::array{
        staged_program{
            .source_device_name = "sda",
            .start_lba = 1500,
            .reserved_sectors = 200,
            .target_path = "/bin/fs_test_runner",
            .required = false,
        },
        // staged_program{
        //     .source_device_name = "sda",
        //     .start_lba = 1000,
        //     .reserved_sectors = 100,
        //     .target_path = "/bin/a",
        //     .required = true,
        // },
        // staged_program{
        //     .source_device_name = "sda",
        //     .start_lba = 1100,
        //     .reserved_sectors = 200,
        //     .target_path = "/bin/eb",
        //     .required = true,
        // },
        // staged_program{
        //     .source_device_name = "sda",
        //     .start_lba = 1300,
        //     .reserved_sectors = 100,
        //     .target_path = "/bin/cat",
        //     .required = true,
        // },
    };

}

// 从"磁盘扇区读入的 ELF 镜像缓冲区"中推导出需要写入文件系统的最小字节数。
// 规则：取所有 PT_LOAD 段中 (offset + filesz) 的最大值。
// 若校验失败或越界则返回 0。
auto static detect_elf_image_size(void const* buf, u32 buf_sz) -> u32
{
    if(!buf || buf_sz < sizeof(elf32::ehdr)) {
        return 0;
    }
    auto const* eh = (elf32::ehdr const*)buf;
    if(memcmp(eh->ident, "\177ELF\1\1\1", 7) != 0) {
        return 0;
    }
    if(eh->phentsize != sizeof(elf32::phdr) || eh->phnum == 0 || eh->phnum > 1024) {
        return 0;
    }
    // 程序头表必须完全在缓冲区内
    u32 const ph_table_end = eh->phoff + (u32)eh->phnum * (u32)eh->phentsize;
    if(ph_table_end > buf_sz) {
        return 0;
    }
    auto const* base = (u8 const*)buf;
    u32 max_end = 0;
    for(u32 i = 0; i < eh->phnum; ++i) {
        auto const* ph = (elf32::phdr const*)(base + eh->phoff + i * eh->phentsize);
        if(ph->type == +elf32::segment::load) {
            u32 const end = ph->offset + ph->filesz;
            if(end > max_end) {
                max_end = end;
            }
        }
    }
    // 没有 PT_LOAD 也给个保底：至少包含程序头表
    if(max_end == 0) {
        max_end = ph_table_end;
    }
    if(max_end > buf_sz) {
        return 0;
    }
    return max_end;
}

// 通用的程序写入函数
auto static write_program_to_fs(staged_program const& program) -> void
{
    auto* source_device = find_block_device(program.source_device_name);
    if(source_device == nullptr) {
        console::println(
            "{}: source device {} not found, skip!",
            program.target_path,
            program.source_device_name
        );
        return;
    }

    auto const reserved_bytes = program.reserved_sectors * SECTOR_SIZE;
    auto program_buf = std::vector(reserved_bytes, char{});

    block_read_blocks(source_device, program.start_lba, program_buf.data(), program.reserved_sectors);

    // 检测 ELF 实际大小
    auto elf_size = detect_elf_image_size(program_buf.data(), reserved_bytes);
    if(elf_size == 0) {
        if(program.required) {
            console::println("{}: invalid ELF format, abort!", program.target_path);
        }
        return;
    }

    // 验证 ELF 大小是否超出预留空间
    if(elf_size > reserved_bytes) {
        console::println(
            "{}: ELF size {} bytes exceeds reserved {} bytes!",
            program.target_path,
            elf_size,
            reserved_bytes
        );
        return;
    }

    // 删除旧文件，避免尾部残留
    unlink(program.target_path);

    auto fd = open(program.target_path, +open_flags::create | +open_flags::rdwr);
    if(fd == -1) {
        console::println("{}: cannot open file for writing!", program.target_path);
        return;
    }

    // 写入完整 ELF 镜像
    auto const write_size = elf_size;
    if(auto wsz = write(fd, program_buf.data(), write_size); (u32)wsz != write_size) {
        console::println(
            "{}: write error! expected {} bytes but wrote {} bytes",
            program.target_path,
            write_size,
            wsz
        );
        close(fd);
        return;
    }

    close(fd);
    console::println("write {} ({} bytes)", program.target_path, write_size);
}

export auto write_execution() -> void
{
    mkdir("/bin");
    for(auto const& program : staged_programs) {
        write_program_to_fs(program);
    }
}
