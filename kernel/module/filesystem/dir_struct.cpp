

export module dir.structure;

import inode.structure;
import filesystem.utility;
import array;

using namespace fs;

// 目录结构
export struct dir
{
    inode* node;
    u32 pos;                    // 记录在目录内的偏移量
    std::array<u8,512> buf;     // 目录的数据缓存
};

export struct dir_entry
{
    std::array<char, MAX_FILES_NAME_LEN> filename;  // 文件名
    u32 inode_no;                                   // 文件或者目录等对应的inode号
    file_type type;                                 // 文件类型
};