

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
};

export auto constexpr operator+(sysid i) -> int
{
    return int(i);
}