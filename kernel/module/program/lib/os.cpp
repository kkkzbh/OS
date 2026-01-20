

export module std.os;

import sys;
export import filesystem.flags;

export namespace std 
{
    // 底层系统调用接口
    using ::std::syscall;
    
    // 进程相关
    inline namespace process
    {
        using ::std::fork;
        using ::std::exec;
        using ::std::getpid;
        using ::std::ps;
        using ::std::wait;
        using ::std::exit;
    }
    
    // 文件 I/O
    inline namespace io
    {
        using ::std::open;
        using ::std::close;
        using ::std::read;
        using ::std::write;
        using ::std::lseek;
        using ::std::unlink;
        using ::std::stat;

        // flags
        using fs::open_flags;
        using fs::whence;
    }
    
    // 目录操作
    inline namespace directory
    {
        using ::std::getcwd;
        using ::std::chdir;
        using ::std::mkdir;
        using ::std::rmdir;
        using ::std::opendir;
        using ::std::closedir;
        using ::std::readdir;
        using ::std::rewinddir;
    }

    // memory
    using ::std::malloc;
    using ::std::free;

}