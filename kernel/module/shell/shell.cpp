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

auto constexpr cmd_len = 128;       // 最大支持键入128个字符的cli输入
auto constexpr MAX_ARG_NR = 16;     // 加上命令名外，最多支持15个参数

// 存储键入的命令
auto cmd_line = std::array<char,cmd_len>{};

// 用来记录当前目录，是当前目录的缓存，每次cd都会更新
auto cwd_cache = std::array<char,64>{};

// 输出提示符
auto prompt() -> void
{
    std::print("[k@kkkzbh {}]$ ", cwd_cache);
}

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
        readline(cmd_line.data(),cmd_line.size());
        if(cmd_line[0] == '\0') {   // 只键入了一个回车
            continue;
        }
    }
    PANIC("shell: should not be here");
}