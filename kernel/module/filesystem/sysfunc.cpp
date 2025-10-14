module;

#include <assert.h>
#include <string.h>

export module filesystem.syscall;

import string;
import string.format;
import filesystem.utility;
import optional;
import console;
import dir;
import path;
import path.structure;
import file;
import file.manager;
import schedule;
import file.structure;
import array;
import algorithm;
import array.format;

// 打开或创建文件成功后，返回文件描述符
export auto open(std::string_view<char const> pathname,open_flags flags) -> optional<i32>
{
    if(pathname.back() == '/') { // 不支持打开目录
        console::println("can't open a directory {}",pathname);
        return {};
    }
    ASSERT(+flags <= 7);
    auto sr = path::search_record{};
    auto depth = path::depth(pathname.data());
    auto found = path::search(pathname.data(),&sr);
    if(sr.type == file_type::directory) {
        console::println("can't open a directory with open(), use opendir() to instead");
        dir_close(sr.parent_dir);
        return {};
    }
    auto cnt = path::depth(sr.path.data());
    // 先判断是否把pathname的各层目录都访问到了，即是否在某个中间目录失败
    if(cnt != depth) { // 说明某个中间目录不存在
        console::println("cannot access {}: Not a directory, subpath {} isn't exist",pathname,sr.path);
        dir_close(sr.parent_dir);
        return {};
    }
    // 如果在最后一个路径上没找到，并且不是要创建文件
    if(not found and not (+flags & +open_flags::create)) {
        console::println("in path {}, file {}  isn't exist",sr.path,strrchr(sr.path.data(),'/') + 1);
        dir_close(sr.parent_dir);
        return {};
    }
    if(found and +flags & +open_flags::create) { // 要创建的文件已经存在
        console::println("{} has already exist!",pathname);
        dir_close(sr.parent_dir);
        return {};
    }
    auto inode_no = *found;
    // 返回的fd是任务pcb->fd_table数组中的元素下标，而非全局file_table中的下标
    switch(+flags & +open_flags::create) {
        case +open_flags::create: {
            console::println("creating file");
            auto fd = file_create(sr.parent_dir,strrchr(pathname.data(),'/') + 1,+flags);
            dir_close(sr.parent_dir);
            return fd;
        } default: {    // 其余情况均为打开已存在文件
            auto fd = file_open(inode_no,flags);
            return fd;
        }
    }

    return {};

}

// 关闭文件
export auto close(i32 fdi) -> bool
{
    if(fdi <= 2) {
        return false;
    }
    auto global_fdi = fdi_local_to_global(fdi);
    auto ret = file_close(&file_table[global_fdi]);
    running_thread()->fd_table[fdi] = -1; // 使这个文件描述符位可用
    return ret;
}

// 将buf中连续count个字节写入文件描述符fd，返回写入的字节数
export auto write(i32 fd,void const* buf,u32 count) -> optional<i32>
{
    if(fd < 0) {
        console::println("sys_write: fd error!");
        return {};
    }
    auto buffer = std::subrange{ (char const*)(buf),(char const*)(buf) + count };
    if(fd == stdout) {
        auto a = std::array<char,1024>{};
        ASSERT(count <= 1024);
        a | std::copy[buffer];
        console::println("{}",a);
        return count;
    }
    auto global_fd = fdi_local_to_global(fd);
    auto& [pos,flag,node] = file_table[global_fd];
    if(+flag & +open_flags::write or +flag & +open_flags::rdwr) {
        return file_write(&file_table[global_fd],buf,count);
    }
    console::println("sys_write: not allowed to write file without flag rdwr or write");
    return {};
}

export auto read(i32 fd,void* buf,u32 count) -> optional<i32>
{
    if(fd < 0) {
        console::println("sys_read: fd error! fd should be greater_equal 0");
        return {};
    }
    ASSERT(buf);
    auto global_fd = fdi_local_to_global(fd);
    return file_read(&file_table[global_fd],buf,count);
}

// 重置用于文件读写操作的偏移指针，成功时返回新的偏移量
export auto lseek(i32 fd,i32 offset,whence flag) -> optional<i32>
{
    if(fd < 0) {
        console::println("sys_lseek: fd:(value {}) error",fd);
        return {};
    }
    auto global_fd = fdi_local_to_global(fd);
    auto pf = &file_table[global_fd];
    auto new_pos = [&] {
        using enum whence;
        switch(flag) {
            case set: {
                return offset;
            } case cur: {
                return i32(pf->pos) + offset;
            } case end: {
                // offset应该是负值
                return i32(pf->node->size) + offset;
            } default: {
                PANIC("whence error!");
            }
        }
        // unreachable
    }(); // NOLINT
    if(new_pos < 0 or new_pos >= i32(pf->node->size)) {
        console::println("the seek pos is error! value({}), which size is {}",new_pos,pf->node->size);
        return {};
    }
    pf->pos = new_pos;
    return pf->pos;
}