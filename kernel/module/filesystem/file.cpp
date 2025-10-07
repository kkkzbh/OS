

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

    auto rollback_step = 0;     // 用于操作失败时回滚资源状态

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