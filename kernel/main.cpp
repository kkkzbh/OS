

export module os;

import kernel;

auto main() -> void
{
    auto fd = *open("/file3",open_flags::rdwr);
    console::println("fd: {}",fd);
    auto str = "Hello World"sv;
    auto cnt = write(fd,str.data(),str.size());
    close(fd);
    console::println("write {} bytes",*cnt);
    console::println("{} close now",fd);
}