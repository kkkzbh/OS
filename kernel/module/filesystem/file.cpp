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
import inode.structure;
import inode;
import algorithm;
import optional;
import scope;
import path.structure;
import path;
import string.format;
import array.format;
import ide;
import buffer;

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
export auto file_open(u32 inode_no, u8 flag) -> optional<i32>
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

// 把buf中的count个字节写入file，成功返回写入的字节数
export auto file_write(file_manager* file,void const* buf,u32 count) -> optional<i32>
{
    // 文件目前最大只支持 512 * 140 = 71680字节
    if((file->node->size + count) > (BLOCK_SIZE * 140)) {
        console::println("exceed max file_size 71680 bytes,write file failed");
        return {};
    }
    auto iobuf = std::vector(512,char{});
    if(not iobuf) {
        console::println("file_write: sys_malloc for iobuf failed");
        return {};
    }
                                // 间接 128块 直接12块
    auto all_blocks = std::vector((BLOCK_SIZE + 48) / sizeof(u32),u32{}); // 用来记录文件所有的块地址
    if(not all_blocks) {
        console::println("file_write: sys_malloc for all_blocks failed");
        return {};
    }

    // 判断文件是否是第一次写，如果是，先为其分配一个块
    if(file->node->sectors[0] == 0) {
        auto v1 = block_bitmap_alloc(cur_part);
        if(not v1) {
            console::println("file_write: block_bitmap_alloc failed");
            return {};
        }
        auto block_lba = *v1;
        file->node->sectors[0] = block_lba;
        // 每分配一个块就同步到硬盘
        auto block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
        ASSERT(block_bitmap_idx);
        bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);
    }

    // 写入count个字节前，该文件已经占用的块数
    auto file_has_used_blocks = file->node->size / BLOCK_SIZE + 1;

    // 存储count个字节后，该文件将占用的块数
    auto file_will_use_blocks = (file->node->size + count) / BLOCK_SIZE + 1;
    ASSERT(file_will_use_blocks <= 140);

    // 通过此增量判断是否需要分配扇区
    auto add_blocks = file_will_use_blocks - file_has_used_blocks;

    // 将写文件所用到的块地址收集到all_blocks，系统中块大小等于扇区大小。
    if(add_blocks == 0) {
        if(file_will_use_blocks <= 12) {  // 如果在12块以内
            auto block_idx = file_has_used_blocks - 1;
            // 指向最后一个已有数据的扇区
            all_blocks[block_idx] = file->node->sectors[block_idx];
        } else {
            // 未写入新数据之前已经占用了间接块，需要将间接块地址读进来
            ASSERT(file->node->sectors.back() != 0);
            auto indirect_block_table = file->node->sectors.back();
            ide_read(cur_part->my_disk, indirect_block_table,all_blocks.data() + 12,1);
        }
    } else {
        // 若有增量，涉及分配新扇区以及是否分配一级间接块表
        // 1. 12个块直接够用
        if(file_will_use_blocks <= 12) {
            // 将有剩余空间的可继续用的扇区地址写入all_blocks
            auto block_idx = file_has_used_blocks - 1;
            ASSERT(file->node->sectors[block_idx] != 0);
            all_blocks[block_idx] = file->node->sectors[block_idx];
            // 再将未来要用的扇区分配好写入all_blocks
            for(auto i : std::iota[file_has_used_blocks,file_will_use_blocks]) {
                auto block_lba = block_bitmap_alloc(cur_part);
                if(not block_lba) {
                    console::println("file_write: block_bitmap_alloc for situation 1 failed");
                    return {};
                }
                // 写文件时，不因该存在块未使用，但已分配扇区的情况，当文件删除，块地址就会清零
                ASSERT(file->node->sectors[i] == 0);    // 确保尚未分配扇区地址
                auto& sectors = file->node->sectors;
                sectors[i] = all_blocks[i] = *block_lba;
                // 每分配一个块就将位图同步到硬盘
                auto block_bitmap_idx = *block_lba - cur_part->sb->data_start_lba;
                bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);
            }
        } else if(file_has_used_blocks <= 12 and file_will_use_blocks > 12) { // 情况 2
            // 先将有剩余空间的可继续用的扇区地址收集到all_blocks
            auto block_idx = file_has_used_blocks - 1;
            auto& sectors = file->node->sectors;
            all_blocks[block_idx] = sectors[block_idx];
            // 创建一级间接块表
            auto block_lba = block_bitmap_alloc(cur_part);
            if(not block_lba) {
                console::println("file_write: block_bitmap_alloc for situation 2 failed");
                return {};
            }
            // 确保一级间接表尚未分配
            ASSERT(sectors.back() == 0);
            // 分配一级间接块索引表
            auto indirect_block_table = sectors.back() = *block_lba;
            for(auto i : std::iota[file_has_used_blocks,file_will_use_blocks]) {
                block_lba = block_bitmap_alloc(cur_part);
                if(not block_lba) {
                    console::println("file_write: block_bitmap_alloc for situation 2 failed");
                    return {};
                }
                if(i < 12) {
                    // 新创建的0 ~ 11块直接存入all_blocks数组
                    ASSERT(sectors[i] == 0);    // 确保尚未分配扇区地址
                    sectors[i] = all_blocks[i] = *block_lba;
                } else {
                    // 间接块只写入到all_blocks数值中，待全部分配完后一次性同步到硬盘
                    all_blocks[i] = *block_lba;
                }
                // 每分配一个块就将位图同步到硬盘
                auto block_bitmap_idx = *block_lba - cur_part->sb->data_start_lba;
                bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);
            }
            // 同步一级间接块表到硬盘
            ide_write(cur_part->my_disk,indirect_block_table,all_blocks.data() + 12,1);
        } else if(file_has_used_blocks > 12) {  // 情况 3
            auto& sectors = file->node->sectors;
            // 确保已经具备了一级间接块表
            ASSERT(sectors.back() != 0);
            // 获取一级间接块表地址
            auto indirect_block_table = sectors.back();
            // 已使用的间接块也将被读入all_blocks，无需单独收录
            ide_read(cur_part->my_disk,indirect_block_table,all_blocks.data() + 12,1); // 获取所有间接块地址
            for(auto i : std::iota[file_has_used_blocks,file_will_use_blocks]) {
                auto block_lba = block_bitmap_alloc(cur_part);
                if(not block_lba) {
                    console::println("file_write: block_bitmap_alloc for situation 3 failed");
                    return {};
                }
                all_blocks[i] = *block_lba;
                // 每分配一个块就将位图同步到硬盘
                auto block_bitmap_idx = *block_lba - cur_part->sb->data_start_lba;
                bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);
            }
            // 同步一级间接块表到硬盘
            ide_write(cur_part->my_disk,indirect_block_table,all_blocks.data() + 12,1);
        }
    }

    // 用到的块地址已经收集到all_blocks中，下面开始写数据
    auto first_write_block = true;  // 含有剩余空间的块标识
    file->pos = file->node->size;   // 记录pos，写文件时实时更新
    auto src = (char const*)(buf);
    auto bytes_written = 0;
    for(auto size_left = count; bytes_written < count; ) {
        iobuf | std::fill[char{}];
        auto sec_idx = file->node->size / BLOCK_SIZE;
        auto sec_lba = all_blocks[sec_idx];
        auto sec_off_bytes = file->node->size % BLOCK_SIZE;
        auto sec_left_bytes = BLOCK_SIZE - sec_off_bytes;
        // 判断此次写入磁盘的数据大小
        auto chunk_size = std::min(size_left,sec_left_bytes);
        if(first_write_block) {
            ide_read(cur_part->my_disk,sec_lba,iobuf.data(),1);
            first_write_block = false;
        }
        memcpy(iobuf.data() + sec_off_bytes,src,chunk_size);
        ide_write(cur_part->my_disk,sec_lba,iobuf.data(),1);
        // console::println("file write at lba {x}",sec_lba);  // 调试信息，可去
        src += chunk_size;
        file->node->size += chunk_size;
        file->pos += chunk_size;
        bytes_written += chunk_size;
        size_left -= chunk_size;
    }
    inode_sync(cur_part,file->node,iobuf.data());
    return bytes_written;
}

// 从文件file中读取count个字节写入buf，返回读取的字节数
export auto file_read(file_manager* file,void* buf,u32 count) -> optional<i32>
{
    auto size_left = count;
    auto size = count;
    // 如果要读取的字节数超过了文件可读的剩余量，就用剩余量作为待读取的字节数
    if((file->pos + count) > file->node->size) {
        size = file->node->size - file->pos;
        size_left = size;
        if(size == 0) {     // 如果没什么读的，到文件尾，直接返回
            return {};
        }
    }
    auto iobuf = std::vector(BLOCK_SIZE,char{});
    if(not iobuf) {
        console::println("file_read: sys_malloc for iobuf failed");
        return {};
    }
    auto all_blocks = std::vector((BLOCK_SIZE + 48) / sizeof(u32),u32{});
    if(not all_blocks) {
        console::println("file_read: sys_malloc for all_blocks failed");
        return {};
    }
    auto block_read_start_idx = file->pos / BLOCK_SIZE; // 数据所在块起始地址
    auto block_read_end_idx = (file->pos + size) / BLOCK_SIZE; // 数据所在块终止地址

    // 如果增量为0
    auto read_blocks = block_read_end_idx - block_read_start_idx;
    ASSERT(block_read_start_idx < 139 and block_read_end_idx < 139);

    if(read_blocks == 0) {   // 在同一扇区内读数据
        ASSERT(block_read_start_idx == block_read_end_idx);
        if(block_read_end_idx < 12) {   // 待读取的数据在12个直接块内
            auto block_idx = block_read_end_idx;
            all_blocks[block_idx] = file->node->sectors[block_idx];
        } else {    // 用到了一级间接块表，需要将表中间接块读进来
            auto indirect_block_table = file->node->sectors.back();
            ide_read(cur_part->my_disk,indirect_block_table,all_blocks.data() + 12,1);
        }
    } else {    // 要读多个块
        if(block_read_end_idx < 12) {   // 数据结束所在的块属于直接块
            for(auto block_idx : std::iota[block_read_start_idx,block_read_end_idx]) {
                all_blocks[block_idx] = file->node->sectors[block_idx];
            }
        } else if(block_read_start_idx < 12 and block_read_end_idx >= 12) {
            // 待读入的数据块跨越直接块和间接块两类
            // 先将直接块地址写入all_blocks
            for(auto i : std::iota[block_read_start_idx,12]) {
                all_blocks[i] = file->node->sectors[i];
            }
            // 确保已经分配了一级间接块表
            ASSERT(file->node->sectors.back() != 0);
            // 再将间接块地址写入all_blocks
            auto indirect_block_table = file->node->sectors.back();
            ide_read(cur_part->my_disk,indirect_block_table,all_blocks.data() + 12,1);
        } else {    // 数据块在间接块中
            // 确保已经分配了一级间接块表
            ASSERT(file->node->sectors.back() != 0);
            auto indirect_block_table = file->node->sectors.back();
            ide_read(cur_part->my_disk,indirect_block_table,all_blocks.data() + 12,1);
        }
    }

    // 用到的块地址已经收集到all_blocks中，下面开始读数据
    auto bytes_read = 0;
    auto a = (char*)(buf);
    while(bytes_read < size) {  // 读完为止
        auto sec_idx = file->pos / BLOCK_SIZE;
        auto sec_lba = all_blocks[sec_idx];
        auto sec_off_bytes = file->pos % BLOCK_SIZE;
        auto sec_left_bytes = BLOCK_SIZE - sec_off_bytes;
        auto chunk_size = std::min(size_left,sec_left_bytes);
        iobuf | std::fill[char{}];
        ide_read(cur_part->my_disk,sec_lba,iobuf.data(),1);
        memcpy(a,iobuf.data() + sec_off_bytes,chunk_size);
        a += chunk_size;
        file->pos += chunk_size;
        bytes_read += chunk_size;
        size_left -= chunk_size;
    }
    return bytes_read;
}