

export module inode.structure;

import utility;
import array;
import list.node;

export struct inode
{

    u32 no;                 // inode编号
    u32 size = 0;               // 文件指文件大小，目录指所有目录项的大小之和

    u32 open_cnts = 0;          // 记录文件被打开的次数
    bool wdeny = false;             // 写文件不能并行，写时要检查该标识

    std::array<u32,13> sectors{}; // [0-11]直接块 12存一级间接块指针 (二三级间接块指针暂不实现)
    list_node tag;         // 用于加入已打开的inode列表，主要用于缓存文件
};

// 用来存储inode的位置
export struct inode_position
{
    bool two_sec;    // inode是否跨越2个扇区
    u32 sec_lba;     // inode所在的扇区号
    u32 off_size;    // inode在扇区内的字节偏移
};
