

export module os;

import kernel;

auto main() -> void
{
    auto fd = *open("/file3",+open_flags::rdwr | +open_flags::create);
    console::println("fd: {}",fd);
    auto str = "Hello World"sv;
    auto cnt = write(fd,str.data(),str.size());
    close(fd);
    console::println("write {} bytes",*cnt);
    console::println("{} close now",fd);

    auto buf = std::array<char,512>{};

    fd = *open("/file3",+open_flags::read);
    console::println("read_fd: {}",fd);
    cnt = read(fd,buf.data(),buf.size());
    console::println("read: {}",buf);
    console::println("the read len is {}",*cnt);
    lseek(fd,2,whence::set);
    buf = {};
    cnt = read(fd,buf.data(),12);
    console::println("re-read: {}",buf);
    close(fd);

    auto ok = unlink("/file3");
    console::println("unline /file3: state {}",ok ? "yes" : "no");

    auto dir = "/kkkzbh/code";
    ok = mkdir(dir);
    console::println("the dir {} create {}",dir,ok ? "successful!" : "failed!");
    auto dir2 = "/kkkzbh/";
    ok = mkdir(dir2);
    console::println("the dir {} create {}",dir2,ok ? "successful!" : "failed!");
    if(not ok) {
        auto dir = "/kkkzbh";
        ok = mkdir(dir);
        console::println("the dir {} create {}",dir,ok ? "successful!" : "failed!");
    }
    auto dir3 = "/kkkzbh/files";
    ok = mkdir(dir3);
    console::println("the dir {} create {}",dir3,ok ? "successful!" : "failed!");
    fd = open("/kkkzbh/files/file1",+open_flags::create | +open_flags::rdwr).value_or(-1);
    if(fd == -1) {
        console::println("open /kkkzbh/files/file1 failed!");
    } else {
        console::println("open the /kkkzbh/files/file1 successful!");
        auto word = "I can create the directory, yeah!\n"sv;
        write(fd,word.data(),word.size());
        lseek(fd,0,whence::set);
        auto a = std::array<char,128>{};
        read(fd,a.data(),word.size());
        console::println("I catch the /kkkzbh/files/file1, it is \n{}",a);
        close(fd);
    }
}