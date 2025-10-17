

export module syscall.utility;

export import utility;

export enum struct sysid : int
{
    getpid,
    write,
    malloc,
    free,
    fork,
    read,
    clear,
    putchar,
    getcwd,
    open,
    close,
    lseek,
    unlink,
    mkdir,
    opendir,
    closedir,
    chdir,
    rmdir,
    readdir,
    rewinddir,
    stat,
    ps
};

export auto constexpr operator+(sysid i) -> int
{
    return int(i);
}