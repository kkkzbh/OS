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

export struct inode
{

    u32 no;                 // inode编号
    u32 size = 0;               // 文件指文件大小，目录指所有目录项的大小之和

    u32 open_cnts = 0;          // 记录文件被打开的次数
    bool wdeny = false;             // 写文件不能并行，写时要检查该标识

    std::array<u32,13> sectors{}; // [0-11]直接块 12存一级间接块指针 (二三级间接块指针暂不实现)
    list::node tag;         // 用于加入已打开的inode列表，主要用于缓存文件
};

// 用来存储inode的位置
struct inode_position
{
    bool two_sec;    // inode是否跨越2个扇区
    u32 sec_lba;     // inode所在的扇区号
    u32 off_size;    // inode在扇区内的字节偏移
};

export auto inode_open(partition* part,u32 inode_no) -> inode*;

// 获取inode所在扇区和扇区内的偏移量
auto inode_locate(partition* part,u32 inode_no) -> inode_position
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