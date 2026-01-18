
export module os;

import kernel;
import test.filesystem;
import inode.structure;
import console;

using namespace fs;

auto func() -> void
{
    auto a = std::vector(50000,int{});
    for(auto i = 0; i != a.size(); ++i) {
        a[i] = i;
    }
    auto fd = open("/test_f",+open_flags::create | +open_flags::write);
    write(fd,a.data(),a.size());
    close(fd);
    fd = open("/test_f",+open_flags::read);
    auto b = std::vector(50000,int{});
    read(fd,b.data(),b.size());
    for(auto i = 0; i != 20; ++i) {
        console::print("{} ",b[i]);
    }
    console::println();
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

auto main() -> void
{
    
}