

export module main;

import kernel;

auto main() -> void
{
    auto fd = *open("/file3",open_flags::read);
    console::println("fd: {}",fd);
    close(fd);
    console::println("{} close now",fd);
}