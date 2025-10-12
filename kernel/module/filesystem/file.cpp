module;

#include <assert.h>
#include <string.h>

export module file;

import filesystem.utility;
import dir;
import string;
import dir.structure;
import console;
import vector;
import file.manager;
import file.structure;
import filesystem;
import inode;
import algorithm;
import optional;
import scope;
import path.structure;
import path;

// 创建文件，成功则返回文件描述符
auto file_create(dir* parent_dir,std::string_view<char const> filename,u8 flag) -> optional<i32>
{
    auto scope_state = true;

    auto iobuf = std::vector(1024,char{});      // 后续操作的公共缓冲区
    if(not iobuf) {
        console::println("in file_create: sys_malloc for iobuf failed!");
        return {};
    }
    auto oino = inode_bitmap_alloc(cur_part);    // 为新文件分配inode
    if(not oino) {
        console::println("in file_create: allocate inode failed!");
        return {};
    }
    auto ino = u32(*oino);


    auto finode = new inode{ ino };
    auto finode_scope = scope_exit {
        [&] {
            cur_part->inode.set(ino,false);
        },
        scope_state
    };
    if(not finode) {
        console::println("file_create: sys_malloc for inode failed!");
        return {};
    }

    auto ofd_idx = get_free_slot_in_global();
    auto ofd_idx_scope = scope_exit {
        [&] {
            delete finode;
        },
        scope_state
    };
    if(not ofd_idx) {
        console::println("exceed max open files!");
        return {};
    }
    auto i = *ofd_idx;

    file_table[i].node = finode;
    file_table[i].pos = 0;
    file_table[i].flag = flag;
    file_table[i].node->wdeny = false;

    auto ndir_entry = dir_entry{};
    create_dir_entry(filename,ino,file_type::regular,&ndir_entry);

    // 同步内存数据到硬盘
    // 在 parent_dir 下安装目录项 ndir_entry
    // 写入硬盘成功后返回true，否则false
    auto sync_scope = scope_exit {
        [&] {
            file_table[i] = {};
        },
        scope_state
    };
    if(not sync_dir_entry(parent_dir,&ndir_entry,iobuf.data())) {
        console::println("sync dir_entry to disk failed!");
        return {};
    }

    iobuf | std::fill[char{}];

    // 将父目录i结点的内容同步到硬盘
    inode_sync(cur_part,parent_dir->node,iobuf.data());

    iobuf | std::fill[char{}];

    // 将新创建文件的i结点内容同步到硬盘
    inode_sync(cur_part,finode,iobuf.data());

    // 将inode_bitmap位图同步到硬盘
    bitmap_sync(cur_part,ino,bitmap_type::inode);

    // 将创建的文件i结点添加到open_inodes链表
    cur_part->open_inodes.push_back(&finode->tag);
    finode->open_cnts = 1;

    scope_state = false;
    return pcb_fd_install(i);

}

// 打开或创建文件成功后，返回文件描述符
auto open(std::string_view<char const> pathname,u8 flags) -> optional<i32>
{
    if(pathname.back() == '/') { // 不支持打开目录
        console::println("can't open a directory {}",pathname);
        return {};
    }
    ASSERT(flags <= 7);
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
    if(not found and not (flags & +open_flags::create)) {
        console::println("in path {}, file {}  isn't exist",sr.path,strrchr(sr.path.data(),'/') + 1);
        dir_close(sr.parent_dir);
        return {};
    }
    if(found and flags & +open_flags::create) { // 要创建的文件已经存在
        console::println("{} has already exist!",pathname);
        dir_close(sr.parent_dir);
        return {};
    }

    // 返回的fd是任务pcb->fd_table数组中的元素下标，而非全局file_table中的下标
    switch(flags & +open_flags::create) {
        case +open_flags::create: {
            console::println("creating file");
            auto fd = file_create(sr.parent_dir,strrchr(pathname.data(),'/') + 1,flags);
            dir_close(sr.parent_dir);
            return fd;
        }
    }

    return {};

}