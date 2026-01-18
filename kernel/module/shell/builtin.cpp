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
import print;
import algorithm;
import array.format;
import stat.structure;
import string.format;

using namespace fs;

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
export auto make_clear_abs_path(char const* path,char* final_path) -> void
{
    auto abs_path = std::array<char,MAX_PATH_LEN>{};
    if (
        path[0] != '/'  // 输入的非绝对路径
        and std::getcwd(abs_path.data(),abs_path.size()) != nullptr // 可以获得cwd
        and not (abs_path[0] == '/' and abs_path[1] == '\0')  // 当前目录非根目录
    ) {
        strcat(abs_path.data(),"/");    // 补上'/'
    }
    strcat(abs_path.data(),path);
    wash_path(abs_path.data(),final_path);
}

// 存储最终绝对路径
export auto final_path = std::array<char,MAX_PATH_LEN>{};

export namespace builtin
{
    auto pwd(u32 argc,char**) -> void
    {
        if(argc != 1) {
            std::print("pwd: no argument support!\n");
            return;
        }
        if(std::getcwd(final_path.data(),final_path.size()) == nullptr) {
            std::print("pwd: get current work directory failed.\n");
        }
        std::print("{}\n",final_path);
    }

    auto cd(u32 argc,char** argv) -> char*
    {
        if(argc > 2) {
            std::print("cd: only support 1 argument!\n");
            return nullptr;
        }
        if(argc == 1) {     // 只是键入cd，则实现是回到根目录
            final_path[0,2] | std::copy["/"sv];
            return final_path.data();
        }
        make_clear_abs_path(argv[1],final_path.data());
        if(not std::chdir(final_path.data())) {
            std::print("cd: no such directory {}\n",final_path);
            return nullptr;
        }
        return final_path.data();
    }

    auto ls(u32 argc,char** argv) -> void
    {
        auto long_info = false;
        auto pathname = (char*)nullptr;
        auto arg_path_num = 0;
        for(auto i : std::iota[1,argc]) {   // 跳过第一个 "ls"
            auto str = std::string_view{ argv[i] };
            if(str.starts_with("-"sv)) {
                if(str == "-l"sv) {
                    long_info = true;
                } else if(str == "-h") {
                    std::print("usage: -l list all information about the file.\n");
                    std::print("-h for help\n");
                    std::print("list all files in the current dirctory if no option\n");
                    return;
                } else {
                    std::print("ls: invalid option {}\n",str);
                    std::print("Try 'ls -h' for more information.\n");
                    return;
                }
            } else {    // 是路径参数
                if(arg_path_num == 0) {
                    arg_path_num = 1;
                    pathname = argv[i];
                } else {
                    std::print("ls: only support one path\n");
                    return;
                }
            }
        }
        if(not pathname) {  // 没有键入路径
            // 以当前路径的绝对路径为参数
            if(std::getcwd(final_path.data(),final_path.size())) {
                pathname = final_path.data();
            } else {
                std::print("ls: getcwd for default ppath failed!\n");
                return;
            }
        } else {    // 解析路径
            make_clear_abs_path(pathname,final_path.data());
            pathname = final_path.data();
        }
        auto file_stat = stat_t{};
        if(not std::stat(pathname,&file_stat)) {
            std::print("ls: cannot access {}: No such file or directory\n",pathname);
            return;
        }
        if(file_stat.type == file_type::directory) {
            auto dir = std::opendir(pathname);
            auto subp = std::array<char,MAX_PATH_LEN>{};
            auto sz = strlen(pathname);
            auto lasti = sz - 1;
            strcpy(subp.data(),pathname);
            if(subp[lasti] != '/') {
                subp[sz] = '/';
                ++sz;
            }
            std::rewinddir(dir);
            if(long_info) {
                std::print("total: {}\n",file_stat.size);
                while(auto dir_e = std::readdir(dir)) {
                    auto ftype = 'd';
                    if(dir_e->type == file_type::regular) {
                        ftype = '-';
                    }
                    subp[sz] = '\0';
                    strcat(&subp[sz],dir_e->filename.data());
                    file_stat = {};
                    if(not std::stat(subp,&file_stat)) {
                        std::print("ls: cannot access {},No such file or directory\n",dir_e->filename);
                        return;
                    }
                    std::print("{}  {}  {}  {}\n",ftype,dir_e->inode_no,file_stat.size,dir_e->filename);
                }
            } else {
                while(auto dir_e = std::readdir(dir)) {
                    std::print("{} ",dir_e->filename);
                }
                std::print("\n");
            }
            std::closedir(dir);
        } else {
            if(long_info) {
                std::print("-  {}  {}  {}\n",file_stat.ino,file_stat.size,pathname);
            } else {
                std::print("{}\n",pathname);
            }
        }
    }

    auto ps(u32 argc,char**) -> void
    {
        if(argc != 1) {
            std::print("ps: no argument support!\n");
            return;
        }
        std::ps();
    }

    auto clear(u32 argc,char**) -> void
    {
        if(argc != 1) {
            std::print("ps: no argument support!\n");
            return;
        }
        std::clear();
    }

    auto mkdir(u32 argc,char** argv) -> bool
    {
        if(argc != 2) {
            std::print("mkdir: only support 1 argument!\n");
            return false;
        }
        make_clear_abs_path(argv[1],final_path.data());
        auto path = std::string_view{ final_path };
        if(path == "/"sv or not std::mkdir(path)) {     // 如果创建是根目录或者失败
            std::print("mkdir: create directory {} failed.\n",argv[1]);
            return false;
        }
        return true;
    }

    auto rmdir(u32 argc,char** argv) -> bool
    {
        if(argc != 2) {
            std::print("rmdir: only support 1 argument!\n");
            return false;
        }
        make_clear_abs_path(argv[1],final_path.data());
        auto path = std::string_view{ final_path };
        if(path == "/"sv or not std::rmdir(path)) {   // 如果删除根目录或者删除失败
            std::print("rmdir: remove {} failed.\n",argv[1]);
            return false;
        }
        return true;
    }

    auto rm(u32 argc,char** argv) -> bool
    {
        if(argc != 2) {
            std::print("rm: only support 1 argument!\n");
            return false;
        }
        make_clear_abs_path(argv[1],final_path.data());
        auto path = std::string_view{ final_path };
        if(path == "/"sv or not std::unlink(path)) {
            std::print("rm: delete {} failed.\n",argv[1]);
            return false;
        }
        return true;

    }

}