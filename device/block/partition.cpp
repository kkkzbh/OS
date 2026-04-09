module;

#include <assert.h>

export module block.partition;

import utility;
import array;
import list;
import bitmap;
import console;
import format;
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

export struct block_partition_table
{
    block_device* device{};
    std::array<partition,4> primary{};
    std::array<partition,8> logical{};
};

export list partition_list;

auto block_partition_tables = std::array<block_partition_table,MAX_BLOCK_DEVICES>{};

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

struct [[gnu::packed]] boot_sector
{
    std::array<u8,446> other;
    std::array<partition_table_entry,4> partition_table;
    u16 signature;
};

export auto init_partition_runtime(partition& part) -> void
{
    part.start_lba = 0;
    part.sec_cnt = 0;
    part.device = nullptr;
    part.part_tag = { nullptr,nullptr };
    for(auto& ch : part.name) {
        ch = '\0';
    }
    part.sb = nullptr;
    part.block = {};
    part.inode = {};
    part.open_inodes.init();
}

auto init_block_partition_table_runtime(block_partition_table& table) -> void
{
    table.device = nullptr;
    for(auto& part : table.primary) {
        init_partition_runtime(part);
    }
    for(auto& part : table.logical) {
        init_partition_runtime(part);
    }
}

auto as_partition(list::node* tag) -> partition*
{
    return (partition*)((u32)tag - (u32)(&((partition*)0)->part_tag));
}

auto reset_partition_slot(partition& part) -> void
{
    if(partition_list.is_initialized() and partition_list.contains(&part.part_tag)) {
        list::erase(&part.part_tag);
    }
    init_partition_runtime(part);
}

auto reset_block_partition_table(block_partition_table& table,block_device* device) -> void
{
    table.device = device;
    for(auto& part : table.primary) {
        reset_partition_slot(part);
    }
    for(auto& part : table.logical) {
        reset_partition_slot(part);
    }
}

auto fill_partition_slot(partition& part,block_device* device,u32 start_lba,u32 sec_cnt,u32 suffix) -> void
{
    part.start_lba = start_lba;
    part.sec_cnt = sec_cnt;
    part.device = device;
    partition_list.push_back(&part.part_tag);
    std::format_to(part.name,"{}{}",device->name,suffix);
}

export auto partition_runtime_init() -> void
{
    partition_list.init();
    for(auto& table : block_partition_tables) {
        init_block_partition_table_runtime(table);
    }
}

export auto find_block_partition_table(block_device* dev) -> block_partition_table*
{
    if(dev == nullptr) {
        return nullptr;
    }
    for(auto idx = u8(0); idx != block_device_cnt; ++idx) {
        if(block_devices[idx] == dev) {
            return &block_partition_tables[idx];
        }
    }
    return nullptr;
}

export auto scan_block_device_partitions(block_device* dev) -> void
{
    ASSERT(dev != nullptr);
    auto table = find_block_partition_table(dev);
    ASSERT(table != nullptr);
    reset_block_partition_table(*table,dev);

    auto primary_no = 0u;
    auto logical_no = 0u;
    auto ext_lba_base = 0u;
    [&](this auto&& self,u32 ext_lba) -> void {
        auto bs = new boot_sector{};
        ASSERT(bs != nullptr);
        block_read_blocks(dev,ext_lba,bs,1);
        for(auto const& entry : bs->partition_table) {
            if(entry.fs_type == 0) {
                continue;
            }
            if(entry.fs_type == 0x5) {
                if(ext_lba_base != 0) [[likely]] {
                    self(entry.start_lba + ext_lba_base);
                } else {
                    ext_lba_base = entry.start_lba;
                    self(entry.start_lba);
                }
                continue;
            }

            if(ext_lba == 0) {
                ASSERT(primary_no < table->primary.size());
                auto slot = primary_no++;
                fill_partition_slot(table->primary[slot],dev,entry.start_lba,entry.sec_cnt,slot + 1);
            } else {
                if(logical_no >= table->logical.size()) {
                    delete bs;
                    return;
                }
                auto slot = logical_no++;
                fill_partition_slot(table->logical[slot],dev,ext_lba + entry.start_lba,entry.sec_cnt,slot + 5);
            }
        }
        delete bs;
    }(0);
}

export auto partition_info(list::node& elem) -> void
{
    auto part = as_partition(&elem);
    console::println(
        "      {} start_lba:0x{x}, sec_cnt:0x{x}",
        part->name,part->start_lba,part->sec_cnt
    );
}
