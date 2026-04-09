module;

#include <assert.h>

export module block.partition;

import utility;
import list;
import bitmap;
import console;
import super_block;
import block.device;

export struct partition
{
    u32 start_lba{};               // 起始扇区
    u32 sec_cnt{};                 // 扇区数
    block_device* device{};        // 分区所属的块设备
    list::node part_tag;           // 于队列的标记
    char name[8]{};                // 分区名称
    super_block* sb{};             // 本分区的超级块
    bitmap block;                  // 块位图
    bitmap inode;                  // i结点位图
    list open_inodes;              // 本分区打开的i结点队列
};

export list partition_list;

export auto init_partition_runtime(partition& part) -> void
{
    part.start_lba = 0;
    part.sec_cnt = 0;
    part.device = nullptr;
    for(auto& ch : part.name) {
        ch = '\0';
    }
    part.sb = nullptr;
    part.block = {};
    part.inode = {};
    part.open_inodes.init();
}

export auto partition_runtime_init() -> void
{
    partition_list.init();
}

export auto partition_info(list::node& elem) -> void
{
    auto pelem = &elem;
    auto part = (partition*)((u32)pelem - (u32)(&((partition*)0)->part_tag));
    console::println(
        "      {} start_lba:0x{x}, sec_cnt:0x{x}",
        part->name,part->start_lba,part->sec_cnt
    );
}
