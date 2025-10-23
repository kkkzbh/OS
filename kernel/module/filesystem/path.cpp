module;

#include <assert.h>
#include <string.h>

export module path;

import algorithm;
import filesystem.utility;
import array;
import path.structure;
import string;
import dir;
import buffer;
import filesystem;
import dir.structure;
import optional;
import console;

namespace path
{
    export auto parse(char const* pathname,char* name_store) -> char const*;
    export auto depth(char const* pathname) -> size_t;
    export auto search(char const* pathname,search_record* search_record) -> optional<int>;
}

namespace path
{
    // 解析最上层的路径昵称
    auto parse(char const* pathname,char* name_store) -> char const*
    {
        while(*pathname == '/') {  // 跳过所有前置/ (针对根目录起手)
            ++pathname;
        }
        while(*pathname != '/' and *pathname) { // 存储目录项昵称
            *name_store++ = *pathname++;
        }
        if(*pathname == '\0') {
            return nullptr;     // 返回空指针表示结束
        }
        return pathname;
    }

    // 返回路径深度
    auto depth(char const* pathname) -> size_t
    {
        ASSERT(pathname != nullptr);
        auto p = pathname;
        auto name = std::array<char,MAX_FILES_NAME_LEN>{};
        auto dep = size_t{};

        while(p) {   // 首项不是'\0'说明含有昵称
            p = parse(p,name.data());
            dep += name[0] != '\0'; // 解析出名字则增加深度
            name | std::fill['\0'];
        }

        return dep;
    }

    // 搜索文件pathname(/开头的全路径)，找到返回inode号
    auto search(char const* pathname,search_record* search_record) -> optional<int>
    {
        auto sv = std::string_view{ pathname };
        if(sv == "/"sv or sv == "/."sv or sv == "/.."sv) {    // 如果查找的是根目录
            search_record->parent_dir = &root;
            search_record->type = file_type::directory;
            search_record->path[0] = '\0';  // 清空搜索路径
            return 0;
        }
        ASSERT(sv.front() == '/' and sv.size() > 1 and sv.size() < MAX_PATH_LEN);  // 确保pathname合法 从/出发
        auto parent_dir = &root;
        auto name = std::array<char,MAX_FILES_NAME_LEN>{};      // 每次解析的单个昵称的缓冲区
        search_record->parent_dir = parent_dir;
        search_record->type = file_type::unknown;
        auto parent_inode_no = 0;
        auto subp = pathname;
        subp = parse(subp,name.data());
        auto buf = std::buffer{ search_record->path };
        auto dir_e = dir_entry{};
        while(name[0]) {    // 能解析出昵称
            ASSERT(buf.size() < MAX_PATH_LEN);
            // 记录已经存在的父目录
            buf += "/";
            buf += name;
            // 在所给的目录中查找文件
            if(search_dir_entry(cur_part,parent_dir,name.data(),&dir_e)) {
                if(dir_e.type == file_type::directory) { // 如果是目录
                    parent_inode_no = parent_dir->node->no;
                    dir_close(parent_dir);
                    parent_dir = dir_open(cur_part,dir_e.inode_no);  // 更新父目录
                    search_record->parent_dir = parent_dir;
                } else if(dir_e.type == file_type::regular) { // 普通文件
                    search_record->type = file_type::regular;
                    return dir_e.inode_no;
                } else {
                    PANIC("find unknow file type!");
                }
                // 目录
                name | std::fill['\0'];
                if(subp) {  // 路径还未结束
                    subp = parse(subp,name.data());
                }
            } else { // 如果找不到
                // 找不到目录项，留着parent_dir不要关闭
                // 若创建新文件在parent_dir中创建
                return {};
            }
        }
        // 到此遍历完路径，只有同名目录存在
        dir_close(search_record->parent_dir);
        // 保存被查找目录的直接父目录
        search_record->parent_dir = dir_open(cur_part,parent_inode_no);
        search_record->type = file_type::directory;
        return dir_e.inode_no;
    }
}