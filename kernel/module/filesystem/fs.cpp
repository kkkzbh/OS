module;

#include <assert.h>

export module filesystem;

import ide;
import utility;
import filesystem.utility;
import inode;
import super_block;
import dir.structure;
import console;
import vector;
import algorithm;
import string;
import list;

export auto format_partition(partition* part) -> void;

export auto mount(list::node* pelem,char const* part_name) -> bool;

auto format_partition(partition* part) -> void
{
    auto constexpr boot_sector_sects = 1; // 引导扇区占用的扇区数
    auto constexpr super_block_sects = 1; // 超级块占用的扇区数
    auto constexpr inode_bitmap_sects = std::div_ceil(MAX_FILES_PER_PART,BITS_PER_SECTOR);
    auto constexpr inode_table_sects = std::div_ceil(sizeof(inode) * MAX_FILES_PER_PART,BLOCK_SIZE);
    auto constexpr used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
    auto free_sects = part->sec_cnt - used_sects;

    // 简单处理块位图占据的扇区数
    auto block_bitmap_sects = std::div_ceil(free_sects,BITS_PER_SECTOR);
    // 位图中位的长度，也是真实可用块的数量
    auto block_bitmap_bit_len = free_sects - block_bitmap_sects;
    // 考虑上位图自己占的扇区，再次更新位图需要占的扇区数(少管理了位图自己占的扇区)
    block_bitmap_sects = std::div_ceil(block_bitmap_bit_len,BITS_PER_SECTOR);

    // 初始化超级块
    auto sb = (super_block) {
        .magic = 0x19590318,
        .sec_cnt = part->sec_cnt,
        .inode_cnt = MAX_FILES_PER_PART,
        .lba_base = part->start_lba,
        .block_bitmap_lba = part->start_lba + 2,  // 第0块是引导块 第1块是超级块
        .block_bitmap_sects = block_bitmap_sects,
        .inode_bitmap_lba = part->start_lba + 2 + block_bitmap_sects,
        .inode_bitmap_sects = inode_bitmap_sects,
        .inode_table_lba = part->start_lba + 2 + block_bitmap_sects + inode_bitmap_sects,
        .inode_table_sects = inode_table_sects,
        .data_start_lba = part->start_lba + 2 + block_bitmap_sects + inode_bitmap_sects + inode_table_sects,
        .root_inode_no = 0,
        .dir_entry_size = sizeof(dir_entry),
    };

    console::println("{} info:", part->name);
    console::println("    magic:0x{x}", sb.magic);
    console::println("    part_lba_base:0x{x}", part->start_lba);
    console::println("    all_sectors:0x{x}", sb.sec_cnt);
    console::println("    inode_cnt:0x{x}", sb.inode_cnt);
    console::println("    block_bitmap_lba:0x{x}", sb.block_bitmap_lba);
    console::println("    block_bitmap_sectors:0x{x}", sb.block_bitmap_sects);
    console::println("    inode_bitmap_lba:0x{x}", sb.inode_bitmap_lba);
    console::println("    inode_bitmap_sectors:0x{x}", sb.inode_bitmap_sects);
    console::println("    inode_table_lba:0x{x}", sb.inode_table_lba);
    console::println("    inode_table_sects:0x{x}", sb.inode_table_sects);
    console::println("    data_start_lba:0x{x}", sb.data_start_lba);

    auto hd = part->my_disk;
    // 将超级块写入本分去的1扇区
    ide_write(hd,part->start_lba + 1,&sb,1);
    console::println("    super_block_lba:0x{x}",part->start_lba + 1);
    // 找出数据量最大的元信息，用其尺寸做存储缓冲区
    auto buf_size = std::max(sb.block_bitmap_sects,sb.inode_bitmap_sects,sb.inode_table_sects) * BLOCK_SIZE;
    auto buf = std::vector(buf_size,u8{});
    // 将块位图初始化并写入sb.block_bitmap_lba
    buf[0] |= 0x01; // 第0个块预留给根目录
    auto block_bitmap_last_byte = block_bitmap_bit_len / 8;
    auto block_bitmap_last_bit = block_bitmap_bit_len % 8;
    // 位图所在的最后一个扇区，不足一扇区的部分
    auto last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE);
    // 先将位图最后一字节到其所在扇区的结束置1 (即因向上取整带来的内存多分配的位)
    // 也就是说 超出实际块数的部分直接置为已占用
    buf[block_bitmap_last_byte,block_bitmap_last_byte + last_size] | std::fill[1];
    // 再将上一步覆盖的最后一字节内的有效位重新置0
    for(auto i = 0; i != block_bitmap_last_bit; ++i) {
        buf[block_bitmap_last_byte] ^= (1 << i);
    }
    ide_write(hd,sb.block_bitmap_lba,buf.data(),sb.block_bitmap_sects);
    // 只清空该清空的地方，提高一点点微弱的效率
    buf[0] = 0;
    buf[block_bitmap_last_byte,buf.size()] | std::fill[0];

    // 将inode位图初始化并写入sb.inode_bitmap_lba
    buf[0] |= 0x1;      // 第0个inode分给根目录
    //规定最大存4096个inode，刚好占1个扇区，所以这个位图不需要额外多处理
    ide_write(hd,sb.inode_bitmap_lba,buf.data(),sb.inode_bitmap_sects);
    buf[0] = 0;

    // 将inode数组初始化并写入sb.inode_table_lba
    auto i = (inode*)buf.data();
    // 写inode_table的第0项
    *i = {
        .no = 0,    // 根目录占第0个inode
        .size = sb.dir_entry_size * 2,  // .和..
        .sectors = { sb.data_start_lba } // 数据的开端是根目录
    };
    ide_write(hd,sb.inode_table_lba,buf.data(),sb.inode_table_sects);
    buf[0,sizeof(inode)] | std::fill[0];

    // 将根目录写入sb.data_start_lba
    auto p_de = (dir_entry*)buf.data();
    std::copy(p_de->filename,"."sv);
    p_de->inode_no = 0;
    p_de->type = file_type::directory;
    ++p_de;
    std::copy(p_de->filename,".."sv);
    p_de->inode_no = 0;     // 根目录的父目录仍然是根目录自己
    p_de->type = file_type::directory;
    // data_start_lba已经分配给根目录，里面是根目录的目录项 目前是. ..
    ide_write(hd,sb.data_start_lba,buf.data(),1);

    console::println("    root_dir_lba:0x{x}",sb.data_start_lba);
    console::println("{} format done",part->name);

}

export partition* cur_part;    // 默认情况下操作的哪儿个分区

// 在分区链表中找到名为part_name的分区，并将其指针赋给cur_part
auto mount(list::node* pelem,char const* part_name) -> bool
{
    auto part = (partition*)((u32)(pelem) - (u32)(&((partition*)0)->part_tag));
    if(std::string_view{ part_name } != std::string_view{ part->name }) {
        return false;
    }
    cur_part = part;
    auto hd = cur_part->my_disk;
    auto sb = new super_block{};
    // 在内存中创建分区cur_part的超级块
    cur_part->sb = new super_block;
    if(cur_part == nullptr or sb == nullptr) {
        PANIC("alloc memory failed!");
    }
    // 读入超级块
    ide_read(hd,cur_part->start_lba + 1,sb,1);;
    // 复制超级块sb到分区的超级块sb中
    cur_part->sb = sb;
    // 将硬盘上的块位图读入到内存
    cur_part->block.bits = new u8[sb->block_bitmap_sects * BLOCK_SIZE];
    if(cur_part->block.bits == nullptr) {
        PANIC("alloc memory failed!");
    }
    cur_part->block.sz = sb->block_bitmap_sects * BLOCK_SIZE;
    // 从硬盘上读入块位图到分区的bits
    ide_read(hd,sb->block_bitmap_lba,cur_part->block.bits,sb->block_bitmap_sects);
    // 将硬盘上的inode位图读入到内存
    cur_part->inode.bits = new u8[sb->inode_bitmap_sects * BLOCK_SIZE];
    if(cur_part->inode.bits == nullptr) {
        PANIC("alloc memory failed!");
    }
    cur_part->inode.sz = sb->inode_bitmap_sects * BLOCK_SIZE;
    // 从硬盘上读入inode位图到分区的inode.bits
    ide_read(hd,sb->inode_bitmap_lba,cur_part->inode.bits,sb->inode_bitmap_sects);
    console::println("mount {} done!",part->name);
    return true;
}