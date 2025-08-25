

export module super_block;

import utility;
import array;


export struct super_block
{
    u32 magic;              // 魔数标识文件系统类型(适用于多文件系统)
    u32 sec_cnt;            // 本分区总共的扇区数
    u32 inode_cnt;          // 本分区inode数量
    u32 lba_base;           // 本分区的起始lba地址

    u32 block_bitmap_lba;   // 块位图的起始扇区地址
    u32 block_bitmap_sects; // 扇区位图占用的扇区地址 (为方便，定义1块 == 1扇区)

    u32 inode_bitmap_lba;   // inode位图的起始扇区lba地址
    u32 inode_bitmap_sects; // inode位图占的扇区数量

    u32 inode_table_lba;    // inode表的起始lba地址
    u32 inode_table_sects;  // inode表占用的扇区数量

    u32 data_start_lba;     // 数据区开始的第一个扇区号
    u32 root_inode_no;      // 根目录所在的inode号
    u32 dir_entry_size;     // 目录项大小

    std::array<u8,460> pad; // 凑够512字节一个扇区

};

