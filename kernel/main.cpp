

export module os;

import kernel;

auto main() -> void
{
    auto fd = *open("/file3",open_flags::rdwr);
    console::println("fd: {}",fd);
    auto str = "Hello World"sv;
    auto cnt = optional{ 0 }; // write(fd,str.data(),str.size());
    close(fd);
    console::println("write {} bytes",*cnt);
    console::println("{} close now",fd);

    auto buf = std::array<char,512>{};

    fd = *open("/file3",open_flags::read);
    console::println("read_fd: {}",fd);
    cnt = read(fd,buf.data(),buf.size());
    console::println("read: {}",buf);
    console::println("the read len is {}",*cnt);
    lseek(fd,2,whence::set);
    buf = {};
    cnt = read(fd,buf.data(),12);
    console::println("re-read: {}",buf);
    close(fd);


}