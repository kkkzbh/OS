

export module syscall.utility;

export import utility;

export enum struct sysid : int
{
    getpid,
};

export auto constexpr operator+(sysid i) -> int
{
    return int(i);
}