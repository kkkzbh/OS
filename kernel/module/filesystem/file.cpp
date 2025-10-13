module;

#include <assert.h>
#include <string.h>
#include <interrupt.h>

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
import string.format;
import array.format;

// 创建文件，成功则返回文件描述符
export auto file_create(dir* parent_dir,std::string_view<char const> filename,u8 flag) -> optional<i32>
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

// 打开编号为inode_no的inode对应的文件，返回文件描述符
export auto file_open(u32 inode_no, open_flags flag) -> optional<i32>
{
    auto fd_idx = get_free_slot_in_global();
    if(not fd_idx) {
        console::println("exceed max open files");
        return {};
    }
    auto fdi = *fd_idx;
    file_table[fdi].node = inode_open(cur_part,inode_no);
    file_table[fdi].pos = 0;    // 每次打开文件将指针为0，即指向文件开头
    file_table[fdi].flag = +flag;
    auto& write_deny = file_table[fdi].node->wdeny;
    if(+flag & +open_flags::write or +flag & +open_flags::rdwr) {
        // 写要考虑是否有其他进程正在写，读不考虑write_deny
        auto old_status = intr_disable();
        if(not write_deny) {   // 当前没有其他进程写这个文件
            write_deny = true;  // 占用该文件
            intr_set_status(old_status);
        } else {    // 若已经有占用，直接失败返回
            intr_set_status(old_status);
            console::println("file can't be write now, the file are being writed, try again later");
            return {};
        }
    }
    // 读或创建，不用考虑write_deny
    return pcb_fd_install(fdi);
}

// 关闭文件，成功返回true，失败返回false
export auto file_close(file_manager* file) -> bool
{
    if(file == nullptr) {
        return false;
    }
    file->node->wdeny = false;
    inode_close(file->node);
    file->node = nullptr;   // 恢复inode状态，使文件结构可用
    return true;
}
