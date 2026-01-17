

export module std.os;

import sys;

export namespace std 
{
    // 底层系统调用接口
    using ::std::syscall;
    
    // 进程相关
    using ::std::fork;
    using ::std::exec;
    using ::std::getpid;
    using ::std::ps;
    
    // 文件 I/O
    using ::std::open;
    using ::std::close;
    using ::std::read;
    using ::std::write;
    using ::std::lseek;
    using ::std::unlink;
    using ::std::stat;
    
    // 目录操作
    using ::std::getcwd;
    using ::std::chdir;
    using ::std::mkdir;
    using ::std::rmdir;
    using ::std::opendir;
    using ::std::closedir;
    using ::std::readdir;
    using ::std::rewinddir;
}