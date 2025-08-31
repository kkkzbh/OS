

export module file;

import filesystem.utility;
import inode;
import array;
import optional;
import algorithm;
import console;
import schedule;
import ide;

struct file
{
    u32 pos;             // 记录当前文件操作的偏移地址，0起始，最大为文件大小-1
    u32 flag;
    inode* node;
};

// 标准输入输出描述符
export enum
{
    stdin,
    stdout,
    stderr
};

// 位图类型
export enum struct bitmap_type
{
    inode,          // inode位图
    block,          // 块位图
};

export auto block_bitmap_alloc(partition* part) -> optional<i32>;

export auto bitmap_sync(partition* part,u32 bi,bitmap_type btmp) -> void;

auto constexpr MAX_FILE_OPEN = 32;      // 系统可打开的最大文件数

auto file_table = std::array<file,MAX_FILE_OPEN>{};

// 从file_table中获取一个空闲位，返回下标
auto get_free_slot_in_global() -> optional<i32>
{
    auto i {
        std::iota[3,MAX_FILE_OPEN]
        | std::first[([](auto i) {
            return file_table[i].node == nullptr;
        })]
        | std::get
    };
    if(i == file_table.size()) {
        console::println("exceed max open files!");
        return {};
    }
    return i;
}

// 将全局描述符表下标安装到进程或线程自己的文件描述符数组fd_table中
// 成功返回下标
auto pcb_fd_install(i32 i) -> optional<i32>
{
    auto cur = running_thread();
    auto local_fd_idx {
        std::iota[3,MAX_FILE_OPEN]
        | std::first[([cur](auto i) {
            return cur->fd_table[i] == -1;  // -1表示free_slot 可用
        })]
    };
    if(not local_fd_idx) {
        console::println("exceed max open files_per_proc!");
        return {};
    }
    return local_fd_idx.get();
}

// 分配一个i结点，返回i结点号
auto inode_bitmap_alloc(partition* part) -> optional<i32>
{
    auto bi = part->inode.scan(1);
    if(not bi) {
        return {};
    }
    part->inode.set(*bi,true);
    return *bi;
}

// 分配1个扇区地址，返回其扇区地址
auto block_bitmap_alloc(partition* part) -> optional<i32>
{
    auto bi = part->block.scan(1);
    if(not bi) {
        return {};
    }
    part->block.set(*bi,true);
    // 返回的不是位图索引，而是具体可用的扇区地址
    return part->sb->data_start_lba + *bi;
}

auto bitmap_sync(partition* part,u32 bi,bitmap_type btmp) -> void
{
    auto off_sec = bi / 4096;               // i结点索引相对于位图的扇区偏移量
    auto off_size = off_sec * BLOCK_SIZE;   // i结点索引相对于位图的字节偏移量
    using enum bitmap_type;
    // 需要被同步到硬盘的位图只有inode和block的bitmap
    switch(btmp) {
        case inode: {
            auto sec_lba = part->sb->inode_bitmap_lba + off_sec;
            auto bitmap_off = part->inode.bits + off_size;
            ide_write(part->my_disk,sec_lba,bitmap_off,1);
            break;
        } case block: {
            auto sec_lba = part->sb->block_bitmap_lba + off_sec;
            auto bitmap_off = part->block.bits + off_size;
            ide_write(part->my_disk,sec_lba,bitmap_off,1);
            break;
        }
    }
}