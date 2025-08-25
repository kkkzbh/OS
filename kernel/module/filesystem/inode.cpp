

export module inode;

import utility;
import list;
import array;

export struct inode
{
    u32 no;                 // inode编号
    u32 size;               // 文件指文件大小，目录指所有目录项的大小之和

    u32 open_cnts;          // 记录文件被打开的次数
    bool wdeny;             // 写文件不能并行，写时要检查该标识

    std::array<u32,13> sectors; // [0-11]直接块 12存一级间接块指针 (二三级间接块指针暂不实现)
    list::node tag;         // 用于加入已打开的inode列表，主要用于缓存文件
};