

export module path.structure;

import dir.structure;
import array;
import filesystem.utility;

namespace path
{
    export struct search_record     // 记录查找文件过程中已找到的上级路径
    {
        std::array<char,MAX_PATH_LEN> path;     // 查找过程的父路径
        dir* parent_dir;        // 文件或目录所在的直接父目录
        file_type type;         // 找到的是普通文件还是目录
    };
}