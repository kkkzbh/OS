module;

#include <assert.h>

export module ata.pio.part;

import list;
import utility;
import array;
import ata.pio;
import block.device;
import block.partition;
import format;

struct [[gnu::packed]] partition_table_entry
{
    u8 bootable;
    u8 start_head;
    u8 start_sec;
    u8 start_chs;
    u8 fs_type;
    u8 end_head;
    u8 end_sec;
    u8 end_chs;

    u32 start_lba;
    u32 sec_cnt;
};

struct boot_sector
{
    std::array<u8,446> other;
    std::array<partition_table_entry,4> partition_table;
    u16 const signature = 0xaa55;
};

export auto scan_partition(ata_pio_device* hd,u32 ext_lba) -> void;

auto ext_lba_base = 0;

auto scan_partition(ata_pio_device* __hd,u32 __ext_lba) -> void
{
    if(__ext_lba == 0) {
        ext_lba_base = 0;
    }
    auto p_no = 0,l_no = 0;
    [&](this auto&& self,ata_pio_device* hd,u32 ext_lba) -> void {
        auto bs = new boot_sector;
        block_read_blocks(&hd->base,ext_lba,bs,1);
        for(auto const& p : bs->partition_table) {
            if(p.fs_type == 0) {
                continue;
            }
            if(p.fs_type == 0x5) {
                if(ext_lba_base != 0) [[likely]] {
                    self(hd,p.start_lba + ext_lba_base);
                } else {
                    ext_lba_base = p.start_lba;
                    self(hd,p.start_lba);
                }
            } else {
                if(ext_lba == 0) {
                    ASSERT(p_no < 4);
                    auto& part = hd->prim_parts[p_no];
                    part.start_lba = ext_lba + p.start_lba;
                    part.sec_cnt = p.sec_cnt;
                    part.device = &hd->base;
                    partition_list.push_back(&part.part_tag);
                    std::format_to(part.name,"{}{}",hd->base.name,++p_no);
                } else {
                    if(l_no >= 8) {
                        return;
                    }
                    auto& part = hd->logic_parts[l_no];
                    part.start_lba = ext_lba + p.start_lba;
                    part.sec_cnt = p.sec_cnt;
                    part.device = &hd->base;
                    partition_list.push_back(&part.part_tag);
                    std::format_to(part.name,"{}{}",hd->base.name,l_no++ + 5);
                }
            }
        }
        delete bs;
    }(__hd,__ext_lba);
}
