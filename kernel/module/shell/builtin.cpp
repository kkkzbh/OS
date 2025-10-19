module;

#include <assert.h>
#include <string.h>

export module shell.builtin;

import filesystem.utility;
import array;
import buffer;
import path;
import string;
import sys;

// 将路径old_abs_path中的..和.转换为实际路径后存入new_abs_path
auto wash_path(char const* old_abs_path,char* new_abs_path) -> void
{
    ASSERT(old_abs_path[0] == '/');
    auto name = std::array<char,MAX_FILES_NAME_LEN>{};
    char const* subp = old_abs_path;
    subp = path::parse(subp,name.data());
    if(name.front() == '\0') {  // 只键入了 '/'
        new_abs_path[0] = '/';
        new_abs_path[1] = '\0';
        return;
    }
    auto buffer = std::buffer{ new_abs_path,0 };
    buffer += "/";
    while(name[0]) {
        auto sv = std::string_view{ name };
        if(sv == ".."sv) {  // 如果是上一级目录
            auto slashp = strrchr(new_abs_path,'/');
            if(slashp != new_abs_path) {    // 如果没到达顶层目录'/'
                *slashp = '\0'; // 等效于回到上一级目录
                buffer.sz = slashp - buffer.buf;    // 重置buffer的大小
            } else {    // 否则就是 '/' 将下一个置0，不可能回退到比'/'还上的
                *(slashp + 1) = '\0';
                buffer.sz = 1;
            }
        } else if(sv != "."sv) {    // 如果不是'.'，将name拼接到new_abs_path
            if(buffer != "/"sv) {   // 防止 "//"
                buffer += "/";
            }
            buffer += name;
        }
        name = {};  // 继续遍历下一层
        if(subp) {
            subp = path::parse(subp,name.data());
        }
    }
}

// 将path处理为不含..和.的绝对路径，存储在final_path
export auto make_clear_abs_path(char const* path,char* final_path) -> void*
{
    auto abs_path = std::array<char,MAX_PATH_LEN>{};
    if (
        path[0] != '/'  // 输入的非绝对路径
        and std::getcwd(abs_path.data(),abs_path.size()) != nullptr // 可以获得cwd
        and abs_path[0] != '/' and abs_path[1] != '\0'  // 当前目录非根目录
    ) {
        strcat(abs_path.data(),"/");    // 补上'/'
    }
    strcat(abs_path.data(),path);
    wash_path(abs_path.data(),final_path);
}