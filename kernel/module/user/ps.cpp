module;

#include <string.h>
#include <assert.h>

export module ps;

import utility;
import format;
import string;
import algorithm;
import file.structure;
import filesystem.syscall;
import list;
import task;
import array;
import schedule;

// 以填充空格的方式输出buf
auto pad_print(char* buf,u32 size,void const* ptr,char format) -> void
{
    memset(buf,0,size);
    u8 out_pad_0idx;
    switch(format) {
        case 's': {
            out_pad_0idx = std::format_to(buf,"{}",(char const*)ptr);
            break;
        } case 'd': {   // 16位
            out_pad_0idx = std::format_to(buf,"{}",*(i16 const*)buf);
            break;
        } case 'x': {
            out_pad_0idx = std::format_to(buf,"{}",*(u32 const*)buf);
            break;
        } default: {
            PANIC("The format is error!");
        }
    }
    std::string_view{ buf,size }[out_pad_0idx,size] | std::fill[' '];   // NOLINT 用空格填充剩余部分
    write(stdout,buf,size - 1);
}

// 用于在list函数中的回调函数，用于针对线程队列的处理
auto thread_info(list::node* pelem) -> void
{
    auto pthread = find_task_by_all(pelem);
    auto out_pad = std::array<char,20>{};
    pad_print(out_pad.data(),out_pad.size(),&pthread->pid,'d');
    if(pthread->parent_pid == -1) {
        pad_print(out_pad.data(),out_pad.size(),"null",'s');
    } else {
        pad_print(out_pad.data(),out_pad.size(),&pthread->parent_pid,'d');
    }
    using enum thread_status;
    switch(pthread->stu) {
        case running: {
            pad_print(out_pad.data(),out_pad.size(),"running",'s');
            break;
        } case ready: {
            pad_print(out_pad.data(),out_pad.size(),"ready",'s');
            break;
        } case blocked: {
            pad_print(out_pad.data(),out_pad.size(),"blocked",'s');
            break;
        } case waiting: {
            pad_print(out_pad.data(),out_pad.size(),"waiting",'s');
            break;
        } case hanging: {
            pad_print(out_pad.data(),out_pad.size(),"hanging",'s');
            break;
        } case died: {
            pad_print(out_pad.data(),out_pad.size(),"died",'s');
            break;
        } default: {
            PANIC("The thread->stu error!");
        }
    }
    pad_print(out_pad.data(),out_pad.size(),&pthread->elapsed_ticks,'x');
    out_pad = {};
    auto sz = strlen(pthread->name);
    ASSERT(sz < 17);
    out_pad[0,sz] | std::copy[pthread->name];
    out_pad[sz,sz + 1] | std::copy["\n"];
    write(stdout,out_pad.data(),sz + 1);
}

auto ps() -> void
{
    auto title = "PID           PPID            STAT        TICKS       COMMAND\n"sv;
    write(stdout,title.data(),title.size());
    thread_all_list | std::apply[thread_info];
}