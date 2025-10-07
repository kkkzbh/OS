
export module file.structure;

import filesystem.utility;
import inode;

export struct file_manager
{
    u32 pos;             // 记录当前文件操作的偏移地址，0起始，最大为文件大小-1
    u32 flag;
    inode* node;
};

// 标准输入输出描述符
export enum
{
    stdin,
    stdout,
    stderr
};

// 位图类型
export enum struct bitmap_type
{
    inode,          // inode位图
    block,          // 块位图
};