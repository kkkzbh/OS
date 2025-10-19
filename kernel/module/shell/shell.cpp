module;

#include <assert.h>

export module shell;

import array;
import print;
import array.format;
import file.structure;
import algorithm;
import sys;
import utility;
import filesystem.utility;
import shell.builtin;

auto constexpr MAX_ARG_NR = 16;     // 加上命令名外，最多支持15个参数

// 存储键入的命令
auto cmd_line = std::array<char,MAX_PATH_LEN>{};
// 存储最终绝对路径
auto final_path = std::array<char,MAX_PATH_LEN>{};

// 用来记录当前目录，是当前目录的缓存，每次cd都会更新
auto cwd_cache = std::array<char,MAX_PATH_LEN>{};

// 输出提示符
auto prompt() -> void
{
    std::print("k@kkkzbh {}$ > ", cwd_cache);
}

// 分析字符串cmd中以token为分隔符的单词，将各单词的指针存入argv数组，返回argc
auto cmd_parse(char* cmd,char** argv,char token) -> i32
{
    ASSERT(cmd);
    for(auto arg_idx : std::iota[MAX_ARG_NR]) {
        argv[arg_idx] = nullptr;
    }
    auto argc = 0;
    for(auto next = cmd; *next; ++argc) {  // 外层循环处理整个命令行
        // 去除命令行或参数之间的token
        while(*next == token) {
            ++next;
        }
        // 处理最后一个参数后接空格的情况，比如 "ls dir2  "
        if(*next == 0) {
            break;
        }
        argv[argc] = next;
        while(*next and *next != token) {   // 内层循环处理命令行中每个命令字及参数
            ++next;
        }
        // 如果是token字符(而非空终止)
        if(*next) {
            *next++ = '\0'; // 令其空终止，作为一个单词的结束，并将next指向下一个字符
        }
        // 避免argv数组访问越界
        if(argc > MAX_ARG_NR) {
            return -1;
        }
    }
    return argc;
}

auto argv = std::array<char*,MAX_ARG_NR>{}; // 全局变量，为了以后exec的程序可访问参数
auto argc = -1;

// 从键盘缓冲区最多读入count个字节到buf
auto readline(char* buf, i32 count) -> void
{
    ASSERT(buf and count > 0);
    auto it = buf;
    while(std::read(stdin,it,1) != -1 and (it - buf) < count) {
        switch(*it) {   // 在不出错的情况下，找到回车符才返回
            case 'l' - 'a': {   // 处理ctrl+l 清屏
                *it = '\0';     // 1. 先将当前的字符置0
                std::clear();   // 2. 清空屏幕
                prompt();       // 3. 打印提示符
                std::print("{}",buf);   // 4. 再将之前键入的内容再次打印
                break;
            } case 'u' - 'a': { // ctrl+u 清掉输入
                while(buf != it) {
                    std::putchar('\b');
                    *it-- = '\0';
                }
                break;
            }
            case '\n':
            case '\r': {
                *it = '\0';    // 添加cmd_line的终止字符
                std::putchar('\n');
                return;
            } case '\b': {
                if(buf[0] != '\b') {    // 阻止删除非本次输入的信息
                    --it;
                    std::putchar('\b');
                }
                break;
            } default: {    // 非控制键则输出字符
                std::putchar(*it++);
            }
        }
    }
    std::print("readline: can't find entry_key in the cmd_line\n");
}

// 简单的shell
export auto shell() -> void
{
    cwd_cache[0] = '/';
    while(true) {
        prompt();
        cmd_line = {};
        final_path = {};
        readline(cmd_line.data(),cmd_line.size());
        if(cmd_line[0] == '\0') {   // 只键入了一个回车
            continue;
        }
        argc = cmd_parse(cmd_line.data(),argv.data(),' ');
        if(argc == -1) {
            std::print("num of arguments exceed {}\n",MAX_ARG_NR);
            continue;
        }
        auto buf = std::array<char,MAX_PATH_LEN>{};
        for(auto arg_idx : std::iota[argc]) {
            make_clear_abs_path(argv[arg_idx],buf.data());
            std::print("{} ",argv[arg_idx]);
        }
        std::print("\n");
    }
    PANIC("shell: should not be here");
}