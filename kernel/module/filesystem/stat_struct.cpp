

export module stat.structure;

import filesystem.utility;

export struct stat_t
{
    u32 ino;        // inode编号
    u32 size;       // 尺寸
    file_type type;    // 文件类型
};

