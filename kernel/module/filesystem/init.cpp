module;

#include <assert.h>
#include <string.h>

export module filesystem.init;

import filesystem.utility;
import super_block;
import console;
import ide;
import algorithm;
import filesystem;
import ide.part;
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
    for(auto i = 0; i != channel_cnt; ++i) {
        for(auto dev_no = 0; dev_no != 2; ++dev_no) {
            if(dev_no == 0) {   // 跨过带OS的裸盘hd64M.img
                continue;
            }
            auto hd = &channels[i].devices[dev_no];
            auto part = hd->prim_parts;
            for(auto part_idx = 0; part_idx != 12; ++part_idx,++part) { // 4主分区 8逻辑
                if(part_idx == 4) { // 逻辑分区
                    part = hd->logic_parts;
                }
                if(part->sec_cnt == 0) { // 分区不存在
                    continue;
                }
                memset(sb,0,sizeof *sb);
                // 读出分区的超级块，根据魔数判断是否存在文件系统
                ide_read(hd,part->start_lba + 1,sb,1);
                // 只支持自己的文件系统，若硬盘上已经有文件系统就不再格式化
                if(sb->magic == 0x19590318) {
                    console::println("{} has filesystem",part->name);
                } else { // 其他文件系统不支持，按无文件系统处理
                    console::println("formatting {}`s partition {}......",hd->name,part->name);
                    format_partition(part);
                }
            }
        }
    }
    auto default_part = "sdb1";
    partition_list | std::apply_until[([=](list::node& nd) {
        return mount(&nd,default_part);
    })];

    // 将当前分区的根目录打开
    open_root_dir(cur_part);

    // 初始化文件表
    for(auto i : std::iota[MAX_FILE_OPEN]) {
        file_table[i].node = nullptr;
    }

    delete sb;
}