module;

#include <assert.h>

export module ide.part;

import list;
import utility;
import array;
import ide;
import format;
import console;

// 分区表项 需要保证强制16字节大小
struct [[gnu::packed]] partition_table_entry
{
    u8 bootable;          // 是否可引导
    u8 start_head;          // 起始磁头号
    u8 start_sec;           // 起始扇区号
    u8 start_chs;           // 起始柱面号
    u8 fs_type;             // 分区类型
    u8 end_head;            // 结束磁头号
    u8 end_sec;             // 结束扇区号
    u8 end_chs;             // 结束柱面号

    u32 start_lba;          // 本分区起始扇区的lba地址
    u32 sec_cnt;            // 本分区的扇区数目
};

// 引导扇区 mbr或者ebr所在的扇区
struct boot_sector
{
    std::array<u8,446> other;       // 引导代码 占位
    std::array<partition_table_entry,4> partition_table; // 4项分区表
    u16 const signature = 0xaa55;      // 启动扇区结束标志的魔数 (0x55aa) 注意小端字节序
};

export auto scan_partition(disk* hd,u32 ext_lba) -> void;

export auto partition_info(list::node& pelem) -> void;

// 用于记录总扩展分区的起始lba，初始为0, partition_scan时以此为标记
auto ext_lba_base = 0;

export list partition_list;       // 用于分区的队列

// 扫描hd中地址为 ext_lba 的扇区的所有分区
auto scan_partition(disk* __hd,u32 __ext_lba) -> void
{
    auto p_no = 0,l_no = 0;     // 用来记录硬盘主分区和逻辑分区的下标
    [&](this auto&& self,disk* hd,u32 ext_lba) -> void {
        auto bs = new boot_sector;
        ide_read(hd,ext_lba,bs,1);
        // 遍历分区表4个分区表项
        for(auto const& p : bs->partition_table) {
            if(p.fs_type == 0) { // 如果分区类型不有效
                continue;   // 跳过
            }
            if(p.fs_type == 0x5) { // 扩展分区
                if(ext_lba_base != 0) [[likely]] {
                // 子扩展分区的start_lba是相对于主引导扇区中的总扩展分区的地址
                self(hd,p.start_lba + ext_lba_base);
                } else { // 第一次读取引导块，也就是主引导记录所在的扇区
                    ext_lba_base = p.start_lba; // 记下总扩展分区的lba地址，作为后续扩展分区的地址参考
                    self(hd,p.start_lba);
                }
            } else { // 如果是非扩展分区
                if(ext_lba == 0) { // 此时全是主分区
                    auto& part = hd->prim_parts[p_no];
                    part.start_lba = ext_lba + p.start_lba;
                    part.sec_cnt = p.sec_cnt;
                    part.my_disk = hd;
                    partition_list.push_back(&part.part_tag);
                    std::format_to(part.name,"{}{}",hd->name,++p_no);
                    ASSERT(p_no < 4); // 0 1 2 3
                } else { // 处理逻辑分区的情况
                    auto& part = hd->logic_parts[l_no];
                    part.start_lba = ext_lba + p.start_lba;
                    part.sec_cnt = p.sec_cnt;
                    part.my_disk = hd;
                    partition_list.push_back(&part.part_tag);
                    std::format_to(part.name,"{}{}",hd->name,l_no++ + 5);
                    if(l_no >= 8) { // 实现上只允许支持8个逻辑分区，数组大小为8
                        return;
                    }
                }
            }
        }
        delete bs;
    }(__hd,__ext_lba);
}

// 打印分区信息
auto partition_info(list::node& elem) -> void
{
    auto pelem = &elem;
    auto part = (partition*)((u32)pelem - (u32)(&((partition*)0)->part_tag));
    console::println (
        "      {} start_lba:0x{x}, sec_cnt:0x{x}",
        part->name,part->start_lba,part->sec_cnt
    );
}