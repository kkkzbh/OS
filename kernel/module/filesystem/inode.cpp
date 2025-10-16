module;

#include <assert.h>
#include <string.h>
#include <interrupt.h>

export module inode;

import utility;
import list;
import array;
import ide;
import filesystem.utility;
import algorithm;
import schedule;
import vector;
import buffer;
import inode.structure;
import file.manager;
import filesystem;
import file.structure;

export auto inode_open(partition* part,u32 inode_no) -> inode*;
export auto inode_close(inode* node) -> void;
export auto inode_sync(partition* part,inode* node,void* buf) -> void;
export auto inode_release(partition* part,u32 inode_no);

// 获取inode所在扇区和扇区内的偏移量
export auto inode_locate(partition* part,u32 inode_no) -> inode_position
{
    // inode_table在硬盘上是连续的
    ASSERT(inode_no < 4096); // 目前仅支持4096个inode

    auto inode_table_lba = part->sb->inode_table_lba; // inode表起始扇区号
    auto constexpr inode_size = sizeof(inode);        // inode大小
    auto off_size = inode_no * inode_size;            // 第inode_no个inode相对于inode表的字节偏移

    auto off_sec = off_size / 512;                     // 第inode_no个inode相对于inode表的扇区偏移
    auto off_size_in_sec = off_size % 512;             // 在扇区内的字节偏移
            // 是否跨扇区                                    // 扇区号                  // 扇区内的字节偏移
    return { off_size_in_sec + inode_size > SECTOR_SIZE, inode_table_lba + off_sec, off_size_in_sec };
}

// 将inode写入到分区part
auto inode_sync(partition* part,inode* node,void* buf) -> void
{
    // buf用于硬盘io的缓冲区
    auto inode_no = node->no;
    auto inode_pos = inode_locate(part,inode_no);
    // inode位置信息在inode_pos中
    ASSERT(inode_pos.sec_lba <= part->start_lba + part->sec_cnt);

    // 硬盘中inode的成员inode_tag以及i_open_cnts无用，主要是在内存中记录链表位置以及被多少进程共享
    auto pure_inode = *node;
    pure_inode.open_cnts = 0;
    pure_inode.wdeny = false;
    pure_inode.tag = {};

    auto ibuf = (char*)buf;
    auto cnt = 1 + inode_pos.two_sec; // 1 or 2
    // 读出所在扇区 写入后 再写回去
    ide_read(part->my_disk,inode_pos.sec_lba,ibuf,cnt);
    memcpy(ibuf + inode_pos.off_size,&pure_inode,sizeof(inode));
    ide_write(part->my_disk,inode_pos.sec_lba,ibuf,cnt);
}

// 根据i结点号返回相应的i结点
auto inode_open(partition* part,u32 inode_no) -> inode*
{
    auto& il = part->open_inodes;
    auto node {
        il
        | std::map[([](list::node& nd) -> inode* {
            return (inode*)((u32)&nd - (u32)(&static_cast<inode*>(0)->tag));
        })]
        | std::first[([=](inode* ind) {
            return ind->no == inode_no;
        })]
    };
    if(node) {
        ++node.get()->open_cnts;
        return node.get();
    }

    // 如果在open_inodes中找不到，就从硬盘上读入此inode并加入到open_inodes链表
    auto inode_pos = inode_locate(part,inode_no);

    // 需要让新inode被所有任务共享，让inode置于内核空间，于是需要临时将cur_pbc->pgdir置为nullptr
    auto cur = running_thread();
    auto cur_pagedir_bak = std::exchange(cur->pgdir,nullptr);
    // 这样分配的nd将源自内核空间
    auto nd = new inode;
    // 恢复pgdir
    cur->pgdir = cur_pagedir_bak;
    auto seccnt = 1 + inode_pos.two_sec;
    auto buf = std::vector(seccnt * BLOCK_SIZE,'\0');
    ide_read(part->my_disk,inode_pos.sec_lba,buf.data(),seccnt);
    memcpy(nd,buf.data() + inode_pos.off_size,sizeof(inode));

    // 在缓存缺失的状态下,从硬盘载入inode,这说明马上要用到inode,插入到队首
    il.push_front(&nd->tag);
    nd->open_cnts = 1;
    return nd;
}

// 关闭inode或减少inode的打开数
auto inode_close(inode* node) -> void
{
    auto old_status = intr_disable();
    // 如果关闭后，已经没有进程打开这个文件，就去掉该inode，把空间释放
    if(--node->open_cnts == 0) {
        list::erase(&node->tag);
        // 同样的套路，从内核空间回收内存
        auto cur = running_thread();
        auto bak = std::exchange(cur->pgdir,nullptr);
        cur->pgdir = nullptr;
        delete node;
        cur->pgdir = bak;
    }
    intr_set_status(old_status);
}

// 将硬盘分区part上的inode清空(数据为0) 主要用于调试，实际无必要
auto inode_delete(partition* part,u32 inode_no,void* buf) -> void
{
    ASSERT(inode_no < 4096);
    auto inode_pos = inode_locate(part,inode_no);
    auto& [two_sec,sec_lba,off_size] = inode_pos;
    ASSERT(sec_lba <= part->start_lba + part->sec_cnt);
    auto buffer = (char*)(buf);
    // 将原硬盘上的内容先读出来
    ide_read(part->my_disk,sec_lba,buffer,1 + two_sec);
    // 将inode清空
    *(inode*)(buffer + off_size) = {};
    // 用清空后的内存数据覆盖硬盘
    ide_write(part->my_disk,sec_lba,buffer,1 + two_sec);
}

// 回收inode的数据块和inode本身
auto inode_release(partition* part,u32 inode_no)
{
    auto node = inode_open(part,inode_no);
    ASSERT(node->no == inode_no);
    // 回收inode占用的所有块
    auto all_blocks = std::array<u32,128 + 12>{};
    for(auto i : std::iota[12]) {
        all_blocks[i] = node->sectors[i];
    }
    auto block_cnt = 12;
    if(node->sectors.back() != 0) {
        block_cnt = 140;
        ide_read(part->my_disk,node->sectors.back(),all_blocks.data() + 12,1);
        // 回收一级间接块表占用的扇区
        auto block_bitmap_idx = node->sectors.back() - part->sb->data_start_lba;
        ASSERT(block_bitmap_idx != 0);
        part->block.set(block_bitmap_idx,false);
        bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);
    }
    for(auto i : std::iota[block_cnt]) {
        if(all_blocks[i] == 0) {
            continue;
        }
        auto block_bitmap_idx = all_blocks[i] - part->sb->data_start_lba;
        ASSERT(block_bitmap_idx > 0);
        part->block.set(block_bitmap_idx,false);
        bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);
    }
    // 回收inode所占用的inode
    part->inode.set(inode_no,false);
    bitmap_sync(cur_part,inode_no,bitmap_type::inode);

    auto iobuf = std::vector(1024,char{});
    inode_delete(part,inode_no,iobuf.data());   // 实现中会对数据清0，实则并不需要
    inode_close(node);
}

