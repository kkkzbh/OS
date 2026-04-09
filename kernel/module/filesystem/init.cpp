module;

#include <assert.h>
#include <string.h>

export module filesystem.init;

import filesystem.utility;
import super_block;
import console;
import block.device;
import block.partition;
import algorithm;
import filesystem;
import list;
import dir;
import file.manager;

export auto filesystem_init() -> void
{
    // 读取存在硬盘上的超级块
    auto sb = new super_block;
    if(sb == nullptr) {
        PANIC("alloc memory failed!");
    }
    console::println("searching filesystem......\n");
    partition_list | std::apply[([&](list::node& nd) {
        auto part = (partition*)((u32)(&nd) - (u32)(&((partition*)0)->part_tag));
        memset(sb,0,sizeof *sb);
        block_read_blocks(part->device,part->start_lba + 1,sb,1);
        if(sb->magic == 0x19590318) {
            console::println("{} has filesystem",part->name);
        } else {
            console::println("formatting {}`s partition {}......",part->device->name,part->name);
            format_partition(part);
        }
    })];
    auto default_part = "sdb1";
    partition_list | std::apply_until[([=](list::node& nd) {
        return mount(&nd,default_part);
    })];
    ASSERT(cur_part != nullptr);

    // 将当前分区的根目录打开
    open_root_dir(cur_part);

    // 初始化文件表
    for(auto i : std::iota[MAX_FILE_OPEN]) {
        file_table[i].node = nullptr;
    }

    delete sb;
}
