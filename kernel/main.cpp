module;

#include <io.h>
#include <interrupt.h>
#include <string.h>

export module os;

import kernel;
import test.filesystem;
import inode.structure;
import console;
import array;

using namespace fs;

// ============ 线程与进程测试 ============

// 延时函数：空循环消耗时间
auto delay(volatile u32 count) -> void
{
    while(count-- > 0) {}
}

// 内核线程A：打印编号并延时
auto k_thread_a(void* arg) -> void
{
    auto cnt = 0;
    while(true) {
        console::println("Thread A: {}", cnt++);
        delay(10000000);  // 延时
    }
}

// 内核线程B：打印编号并延时
auto k_thread_b(void* arg) -> void
{
    auto cnt = 0;
    while(true) {
        console::println("Thread B: {}", cnt++);
        delay(10000000);  // 延时
    }
}

// 测试内核线程并发调度
auto test_kernel_threads() -> void
{
    console::println("=== Testing Kernel Threads ===");
    thread_start("k_thread_a", 31, k_thread_a, nullptr);
    thread_start("k_thread_b", 8, k_thread_b, nullptr);
    // main 线程也循环打印
    auto cnt = 0;
    while(true) {
        console::println("Main: {}", cnt++);
        delay(10000000);
    }
}

// 用户进程函数
auto u_prog_a() -> void
{
    while(true) {
        // 用户态进程循环
    }
}

// 用户进程测试特权级
auto u_prog_privilege_test() -> void
{
    // 尝试执行特权指令 cli（关中断）
    // 在 Ring 3 下应触发 #GP 异常
    asm volatile("cli");
    // 如果执行到这里说明特权级保护失效
    while(true) {}
}

// 测试用户进程
auto test_user_process() -> void
{
    console::println("=== Testing User Process ===");
    process_execute((void*)u_prog_a, "user_prog_a");
    console::println("User process created, entering main loop");
    auto cnt = 0;
    while(true) {
        console::println("Main: {}", cnt++);
        delay(10000000);
    }
}

// 测试用户进程特权级保护（预期会触发 #GP 异常）
auto test_user_privilege() -> void
{
    console::println("=== Testing User Privilege Protection ===");
    console::println("Creating user process that will try 'cli' instruction...");
    console::println("Expected: #GP General Protection Exception");
    process_execute((void*)u_prog_privilege_test, "privilege_test");
    while(true) {}
}

// 测试 malloc/free 功能
auto test_malloc_free() -> void
{
    console::println("=== Testing malloc/free ===");
    
    // 测试1: 小块分配 (16B ~ 1024B，走 Arena)
    console::println("[1] Small block allocation (Arena):");
    auto p1 = malloc(32);
    auto p2 = malloc(64);
    auto p3 = malloc(128);
    console::println("  malloc(32)  = 0x{x}", (u32)p1);
    console::println("  malloc(64)  = 0x{x}", (u32)p2);
    console::println("  malloc(128) = 0x{x}", (u32)p3);
    
    // 测试2: 大块分配 (>1024B，直接分页)
    console::println("[2] Large block allocation (Page):");
    auto p4 = malloc(2048);
    auto p5 = malloc(4096);
    console::println("  malloc(2048) = 0x{x}", (u32)p4);
    console::println("  malloc(4096) = 0x{x}", (u32)p5);
    
    // 测试3: 写入数据验证
    console::println("[3] Write/Read verification:");
    auto arr = (int*)malloc(10 * sizeof(int));
    for(int i = 0; i < 10; ++i) {
        arr[i] = i * 100;
    }
    console::print("  Data: ");
    for(int i = 0; i < 10; ++i) {
        console::print("{} ", arr[i]);
    }
    console::println();
    
    // 测试4: 释放内存
    console::println("[4] Free memory:");
    free(p1);
    free(p2);
    free(p3);
    free(p4);
    free(p5);
    free(arr);
    console::println("  All freed successfully!");
    
    // 测试5: 释放后重新分配（验证复用）
    console::println("[5] Reallocation after free:");
    auto p6 = malloc(32);
    auto p7 = malloc(64);
    console::println("  malloc(32) again = 0x{x}", (u32)p6);
    console::println("  malloc(64) again = 0x{x}", (u32)p7);
    free(p6);
    free(p7);
    
    console::println("=== malloc/free test PASSED ===");
}

// 综合文件系统测试
auto test_filesystem() -> void
{
    console::println("=== Testing Filesystem ===");

    // 1. 文件基本读写测试
    console::println("[1] File Write/Read Test");
    unlink("/file1"); // 清理旧文件
    
    auto fd = open("/file1", +open_flags::create | +open_flags::write);
    if(fd != -1) {
        auto msg = "hello, world!";
        write(fd, msg, 13);
        close(fd);
        console::println("    Created /file1 and wrote 'hello, world!'");
    } else {
        console::println("    Failed to create /file1");
    }

    fd = open("/file1", +open_flags::read);
    if(fd != -1) {
        char buf[32] = {};
        auto len = read(fd, buf, 32);
        close(fd);
        console::println("    Read from /file1: {} (len={})", buf, len);
    } else {
        console::println("    Failed to open /file1");
    }

    // 2. 目录操作测试
    console::println("[2] Directory Operation Test");
    
    // 清理旧目录（先删除其中的文件，再删除目录）
    unlink("/dir1/.test");
    unlink("/dir1/file2");
    rmdir("/dir1");
    
    if(mkdir("/dir1")) {
        console::println("    Created directory /dir1");
    }
    
    fd = open("/dir1/.test", +open_flags::create | +open_flags::write);
    if(fd != -1) {
        close(fd);
        console::println("    Created file /dir1/.test");
    }

    // 在目录中创建另一个文件
    fd = open("/dir1/file2", +open_flags::create | +open_flags::write);
    if(fd != -1) {
        close(fd);
        console::println("    Created file /dir1/file2");
    }
    
    // 列出当前目录内容
    console::println("    Listing /dir1 content:");
    auto dir = opendir("/dir1");
    if(dir) {
        while(auto de = readdir(dir)) {
            console::println("      - {}", de->filename.data());
        }
        closedir(dir);
    }

    console::println("=== Filesystem Test Done ===");
}

// 测试 file_read 是否正常工作
auto test_file_read() -> void
{
    console::println("=== Testing file_read ===");
    
    // 先删除旧的测试文件（如果存在）
    unlink("/test_read");
    
    auto test_data = "Hello, this is a test file content for file_read!";
    auto fd = open("/test_read", +open_flags::create | +open_flags::write);
    if(fd == -1) {
        console::println("Failed to create /test_read");
        return;
    }
    write(fd, test_data, 50);
    close(fd);
    console::println("Created /test_read and wrote 50 bytes");
    
    fd = open("/test_read", +open_flags::read);
    if(fd == -1) {
        console::println("Failed to open /test_read for reading");
        return;
    }
    
    char buf[64] = {};
    auto bytes_read = read(fd, buf, 50);
    close(fd);
    
    console::println("Read {} bytes: {}", bytes_read, std::string_view{buf, (size_t)bytes_read});
    
    bool match = true;
    for(int i = 0; i < 50; ++i) {
        if(buf[i] != test_data[i]) {
            match = false;
            console::println("Mismatch at byte {}", i);
            break;
        }
    }
    
    // 清理测试文件
    unlink("/test_read");
    
    if(match) {
        console::println("=== file_read test PASSED ===");
    } else {
        console::println("=== file_read test FAILED ===");
    }
}

// 测试 console::println 各种参数组合
auto test_console() -> void
{
    console::println("=== Testing console::println ===");
    console::println("Hello World! {} {}",23,"qwe");
    console::println("Hello World! {} {}",23,12);
    console::println("fffffffff {}", 12);
    console::println("fffffffff {}", "Hello"sv);
    console::println("fffffffff {}", "Hello");
    console::println("=== console test done ===");
}

// ============ 第七章设备驱动综合测试 ============



// 创建一个演示用的文本文件
auto create_readme() -> void
{
    unlink("/readme.txt");
    auto fd = open("/readme.txt", +open_flags::create | +open_flags::write);
    if(fd != -1) {
        auto constexpr static content =
            "Who knows how many git commits were pushed at 4 AM...\n"
            "Countless page faults... Stack overflow? Memset reversed? Link error?\n"
            "Facing this vast repo, debugging often felt like a losing battle.\n"
            "Only those who've walked this path understand the hardship:\n"
            "    Tracing the execution flow from zero, with a trembling heart.\n\n"
            "Anyway, I persisted to this moment.\n"
            "    Even though some mysteries still linger in this OS.\n"
            "But now, it is time to say goodbye.\n"sv;
        write(fd, content.data(), content.size());
        close(fd);
        console::println("Created /readme.txt for demo.");
    }
}

auto main() -> void
{
    // create_readme();

    while(true) {
    }
}