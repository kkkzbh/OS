

export module test.filesystem;

import kernel;

export namespace test
{
    // 测试 打开 关闭 删除 写入 文件
    auto t1() -> void
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
    }

    // 测试 mkdir 然后在目录里建文件 写文件 读文件 关闭文件
    auto t2() -> void
    {
        auto dir = "/kkkzbh/code";
        auto ok = mkdir(dir);
        console::println("the dir {} create {}",dir,ok ? "successful!" : "failed!");
        auto dir2 = "/kkkzbh";
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
        auto fd = open("/kkkzbh/files/file1",+open_flags::create | +open_flags::rdwr).value_or(-1);
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

    // 测试打开关闭目录
    auto t3() -> void
    {
        auto constexpr dirname = "/kkkzbh/code"sv;
        auto pdir = opendir(dirname);
        if(not pdir) {
            console::println("can not open dir {}",dirname);
            return;
        }
        console::println("open {}} successful!",dirname);
        auto ok = closedir(pdir);
        if(not ok) {
            console::println("{} close failed!",dirname);
        } else {
            console::println("{} close successful!",dirname);
        }
    }

    // 主要测试读目录项
    auto t4() -> void
    {
        auto dirname = "/kkkzbh/files"sv;
        auto pdir = opendir(dirname);
        if(not pdir) {
            console::println("can't open {}",dirname);
            return;
        }
        console::println("open {} successful!",dirname);
        for(auto dir_e = (dir_entry*)(nullptr); (dir_e = readdir(pdir)); ) {
            auto type = [&] {
                if(dir_e->type == file_type::regular) {
                    return "regular"sv;
                }
                return "directory"sv;
            }();
            console::println("    {}    {}",type,dir_e->filename);
        }
        auto ok = closedir(pdir);
        if(ok) {
            console::println("close {} successful!",dirname);
        } else {
            console::println("close {} failed!",dirname);
        }
    }

    // 测试删除目录
    auto t5() -> void
    {
        console::println("xxx:> ls /kkkzbh");
        auto dir = opendir("/kkkzbh");
        for(auto dir_e = (dir_entry*)(nullptr); (dir_e = readdir(dir)); ) {
            auto type = [&] {
                if(dir_e->type == file_type::regular) {
                    return "regular"sv;
                }
                return "directory"sv;
            }();
            console::println("    {}    {}",type,dir_e->filename);
        }
        console::println("xxx:> ls /kkkzbh/files");
        auto dir2 = opendir("/kkkzbh/files");
        for(auto dir_e = (dir_entry*)(nullptr); (dir_e = readdir(dir2)); ) {
            auto type = [&] {
                if(dir_e->type == file_type::regular) {
                    return "regular"sv;
                }
                return "directory"sv;
            }();
            console::println("    {}    {}",type,dir_e->filename);
        }
        console::println("xxx:> rmdir /kkkzbh/files");
        auto ok = rmdir("/kkkzbh/files");
        console::println("xxx:> mkdir /kkkzbh/empty");
        auto ok2 = mkdir("/kkkzbh/empty");
        console::println("xxx:> ls /kkkzbh");
        rewinddir(dir);
        for(auto dir_e = (dir_entry*)(nullptr); (dir_e = readdir(dir)); ) {
            auto type = [&] {
                if(dir_e->type == file_type::regular) {
                    return "regular"sv;
                }
                return "directory"sv;
            }();
            console::println("    {}    {}",type,dir_e->filename);
        }
        console::println("xxx:> rmdir /kkkzbh/empty");
        rmdir("/kkkzbh/empty");
        console::println("xxx:> ls /kkkzbh");
        rewinddir(dir);
        for(auto dir_e = (dir_entry*)(nullptr); (dir_e = readdir(dir)); ) {
            auto type = [&] {
                if(dir_e->type == file_type::regular) {
                    return "regular"sv;
                }
                return "directory"sv;
            }();
            console::println("    {}    {}",type,dir_e->filename);
        }
    }
}

