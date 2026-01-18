

export module filesystem.flags;

import utility;

export namespace fs
{
    enum struct file_type : u8
    {
        unknown,
        regular,
        directory
    };

    enum struct open_flags : u8      // 打开文件的选项
    {
        read,
        write,
        rdwr,
        create = 4
    };

    enum struct whence : u8
    {
        set,
        cur,
        end
    };

    auto constexpr operator+(open_flags flags)
    {
        return __underlying_type(open_flags)(flags);
    }

    auto constexpr operator|(open_flags a,open_flags b) -> open_flags
    {
        return open_flags(+a | +b);
    }

    auto constexpr operator&(open_flags a,open_flags b) -> bool
    {
        return (+a & +b) != 0;
    }

    auto constexpr operator+(whence flags)
    {
        return __underlying_type(whence)(flags);
    }
}
