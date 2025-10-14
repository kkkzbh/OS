

export module filesystem.utility;

export import utility;

export auto constexpr MAX_FILES_NAME_LEN = 16;          // 最大文件名长度;

export auto constexpr MAX_FILES_PER_PART = 4096;        // 每个分区最大的文件数

export auto constexpr BITS_PER_SECTOR = 4096;           // 每扇区位数

export auto constexpr SECTOR_SIZE = 512;                // 每扇区字节大小

export auto constexpr BLOCK_SIZE = SECTOR_SIZE;         // 每块大小(1扇区)

export auto constexpr MAX_PATH_LEN = 512;               // 最大支持的路径长度

export enum struct file_type : u8
{
    unknown,
    regular,
    directory
};

export enum struct open_flags : u8      // 打开文件的选项
{
    read,
    write,
    rdwr,
    create = 4
};

export enum struct whence : u8
{
    set,
    cur,
    end
};

export auto constexpr operator+(open_flags flags)
{
    return __underlying_type(open_flags)(flags);
}

export auto constexpr operator+(whence flags)
{
    return __underlying_type(whence)(flags);
}