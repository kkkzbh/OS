module;

#include <interrupt.h>

export module storage.regression;

import block.device;
import block.partition;
import console;
import array;
import format;
import list;
import utility;
import vector;

export auto disk_regression_mode_requested() -> bool;
export auto disk_regression_mode_active() -> bool;
export auto disk_regression_mode_case() -> char const*;
export auto run_disk_regression(char const* case_name) -> bool;
export auto disktest(char const* case_name) -> int;

namespace
{
    auto constexpr sector_size = u32(512);

    auto constexpr read_sector_case = "read_sector";
    auto constexpr cross_sector_case = "cross_sector_read";
    auto constexpr read_after_write_case = "read_after_write";
    auto constexpr partition_table_scan_case = "partition_table_scan";
    auto constexpr multi_sector_read_case = "multi_sector_read";
    auto constexpr max_transfer_boundary_read_case = "max_transfer_boundary_read";
    auto constexpr multi_sector_write_case = "multi_sector_write";
    auto constexpr disk_suite_marker_lba = u32(1799);
    auto constexpr disk_suite_marker = "DISKTEST";
    auto constexpr disk_suite_case_offset = u32(16);
    auto constexpr disk_suite_case_capacity = u32(32);

    auto constexpr read_sector_lba = u32(1800);
    auto constexpr cross_sector_lba = u32(1801);
    auto constexpr scratch_sector_lba = u32(1803);
    auto constexpr multi_sector_read_lba = u32(1804);
    auto constexpr multi_sector_read_cnt = u32(16);
    auto constexpr max_transfer_boundary_lba = multi_sector_read_lba + multi_sector_read_cnt;
    auto constexpr max_transfer_boundary_cnt = u32(257);
    auto constexpr multi_sector_write_lba = max_transfer_boundary_lba + max_transfer_boundary_cnt;
    auto constexpr multi_sector_write_cnt = u32(8);

    auto constexpr seed_salt = u8(0x11);
    auto constexpr write_salt = u8(0x5a);
    auto disk_regression_mode = false;
    auto disk_regression_case_name = std::array<char, disk_suite_case_capacity>{};

    auto str_eq(char const* lhs,char const* rhs) -> bool
    {
        for(auto i = 0;; ++i) {
            if(lhs[i] != rhs[i]) {
                return false;
            }
            if(lhs[i] == '\0') {
                return true;
            }
        }
    }

    auto bytes_eq(char const* lhs,char const* rhs,u32 count) -> bool
    {
        for(auto i = 0u; i != count; ++i) {
            if(lhs[i] != rhs[i]) {
                return false;
            }
        }
        return true;
    }

    auto pattern_byte(u32 lba,u32 byte_off,u8 salt) -> char
    {
        return char((u32(salt) + ((lba * 17) & 0xff) + byte_off) & 0xff);
    }

    auto fill_pattern(char* dst,u32 start_lba,u32 sector_count,u8 salt) -> void
    {
        for(auto sec = 0u; sec != sector_count; ++sec) {
            auto const lba = start_lba + sec;
            for(auto byte_off = 0u; byte_off != sector_size; ++byte_off) {
                dst[sec * sector_size + byte_off] = pattern_byte(lba,byte_off,salt);
            }
        }
    }

    auto matches_pattern(char const* buf,u32 start_lba,u32 sector_count,u8 salt) -> bool
    {
        for(auto sec = 0u; sec != sector_count; ++sec) {
            auto const lba = start_lba + sec;
            for(auto byte_off = 0u; byte_off != sector_size; ++byte_off) {
                if(buf[sec * sector_size + byte_off] != pattern_byte(lba,byte_off,salt)) {
                    return false;
                }
            }
        }
        return true;
    }

    auto fail(char const* case_name,char const* reason) -> bool
    {
        console::println("DISKCASE:{}:FAIL:{}",case_name,reason);
        return false;
    }

    auto pass(char const* case_name) -> bool
    {
        console::println("DISKCASE:{}:PASS",case_name);
        return true;
    }

    auto find_required_device(char const* name,char const* case_name) -> block_device*
    {
        auto* dev = find_block_device(name);
        if(dev == nullptr) {
            auto reason = std::array<char,96>{};
            std::format_to(reason.data(),"device {} not found",name);
            fail(case_name,reason.data());
        }
        return dev;
    }

    auto is_known_case(char const* case_name) -> bool
    {
        return str_eq(case_name,read_sector_case) or
               str_eq(case_name,cross_sector_case) or
               str_eq(case_name,read_after_write_case) or
               str_eq(case_name,partition_table_scan_case) or
               str_eq(case_name,multi_sector_read_case) or
               str_eq(case_name,max_transfer_boundary_read_case) or
               str_eq(case_name,multi_sector_write_case);
    }

    auto check_partition(partition const& part,char const* name,u32 start_lba,u32 sec_cnt) -> bool
    {
        return str_eq(part.name,name) and part.start_lba == start_lba and part.sec_cnt == sec_cnt and part.device != nullptr;
    }

    auto check_empty_partition(partition const& part) -> bool
    {
        return part.device == nullptr and part.start_lba == 0 and part.sec_cnt == 0 and part.name[0] == '\0';
    }

    auto as_partition(list::node& tag) -> partition*
    {
        return (partition*)((u32)&tag - (u32)(&((partition*)0)->part_tag));
    }

    auto check_partition_list(char const* case_name) -> bool
    {
        struct expected_partition
        {
            char const* name;
            u32 start_lba;
            u32 sec_cnt;
        };

        auto constexpr expected = std::array{
            expected_partition{ "sdb1", 0x800, 0x8000 },
            expected_partition{ "sdb5", 0x9000, 0x4800 },
            expected_partition{ "sdb6", 0xE000, 0x6000 },
            expected_partition{ "sdb7", 0x14800, 0x3800 },
            expected_partition{ "sdb8", 0x18800, 0x7800 },
            expected_partition{ "sdb9", 0x20800, 0x75E0 },
        };

        auto idx = 0u;
        for(auto& elem : partition_list) {
            if(idx >= expected.size()) {
                return fail(case_name,"partition_list contains unexpected extra entries");
            }
            auto* part = as_partition(elem);
            auto const& want = expected[idx];
            if(not check_partition(*part,want.name,want.start_lba,want.sec_cnt)) {
                auto reason = std::array<char,96>{};
                std::format_to(reason.data(),"partition_list entry {} mismatch",idx);
                return fail(case_name,reason.data());
            }
            ++idx;
        }

        if(idx != expected.size()) {
            auto reason = std::array<char,96>{};
            std::format_to(reason.data(),"partition_list expected {} entries got {}",expected.size(),idx);
            return fail(case_name,reason.data());
        }
        return true;
    }

    auto case_read_sector() -> bool
    {
        auto* sda = find_required_device("sda",read_sector_case);
        if(sda == nullptr) {
            return false;
        }
        auto buf = std::array<char,sector_size>{};
        block_read_blocks(sda,read_sector_lba,buf.data(),1);
        if(not matches_pattern(buf.data(),read_sector_lba,1,seed_salt)) {
            return fail(read_sector_case,"sector pattern mismatch");
        }
        return pass(read_sector_case);
    }

    auto case_cross_sector_read() -> bool
    {
        auto* sda = find_required_device("sda",cross_sector_case);
        if(sda == nullptr) {
            return false;
        }
        auto buf = std::array<char,sector_size * 2>{};
        block_read_blocks(sda,cross_sector_lba,buf.data(),2);
        if(not matches_pattern(buf.data(),cross_sector_lba,2,seed_salt)) {
            return fail(cross_sector_case,"cross-sector pattern mismatch");
        }
        return pass(cross_sector_case);
    }

    auto case_read_after_write() -> bool
    {
        auto* sda = find_required_device("sda",read_after_write_case);
        if(sda == nullptr) {
            return false;
        }
        auto backup = std::array<char,sector_size>{};
        auto expected = std::array<char,sector_size>{};
        auto verify = std::array<char,sector_size>{};
        auto restored = std::array<char,sector_size>{};

        block_read_blocks(sda,scratch_sector_lba,backup.data(),1);
        fill_pattern(expected.data(),scratch_sector_lba,1,write_salt);
        block_write_blocks(sda,scratch_sector_lba,expected.data(),1);
        block_read_blocks(sda,scratch_sector_lba,verify.data(),1);
        block_write_blocks(sda,scratch_sector_lba,backup.data(),1);
        block_read_blocks(sda,scratch_sector_lba,restored.data(),1);

        if(not bytes_eq(verify.data(),expected.data(),sector_size)) {
            return fail(read_after_write_case,"write/readback mismatch");
        }
        if(not bytes_eq(restored.data(),backup.data(),sector_size)) {
            return fail(read_after_write_case,"restore mismatch");
        }
        return pass(read_after_write_case);
    }

    auto case_partition_table_scan() -> bool
    {
        auto* sdb = find_required_device("sdb",partition_table_scan_case);
        if(sdb == nullptr) {
            return false;
        }

        scan_block_device_partitions(sdb);
        auto* table = find_block_partition_table(sdb);
        if(table == nullptr) {
            return fail(partition_table_scan_case,"partition table missing");
        }
        if(not check_partition(table->primary[0],"sdb1",0x800,0x8000)) {
            return fail(partition_table_scan_case,"sdb1 mismatch");
        }
        if(not check_empty_partition(table->primary[1])) {
            return fail(partition_table_scan_case,"primary slot 2 should be empty");
        }
        if(not check_empty_partition(table->primary[2])) {
            return fail(partition_table_scan_case,"primary slot 3 should be empty");
        }
        if(not check_empty_partition(table->primary[3])) {
            return fail(partition_table_scan_case,"primary slot 4 should be empty");
        }
        if(not check_partition(table->logical[0],"sdb5",0x9000,0x4800)) {
            return fail(partition_table_scan_case,"sdb5 mismatch");
        }
        if(not check_partition(table->logical[1],"sdb6",0xE000,0x6000)) {
            return fail(partition_table_scan_case,"sdb6 mismatch");
        }
        if(not check_partition(table->logical[2],"sdb7",0x14800,0x3800)) {
            return fail(partition_table_scan_case,"sdb7 mismatch");
        }
        if(not check_partition(table->logical[3],"sdb8",0x18800,0x7800)) {
            return fail(partition_table_scan_case,"sdb8 mismatch");
        }
        if(not check_partition(table->logical[4],"sdb9",0x20800,0x75E0)) {
            return fail(partition_table_scan_case,"sdb9 mismatch");
        }
        for(auto idx = 5u; idx != 8; ++idx) {
            if(not check_empty_partition(table->logical[idx])) {
                return fail(partition_table_scan_case,"unused logical partition slot should be empty");
            }
        }
        if(not check_partition_list(partition_table_scan_case)) {
            return false;
        }

        scan_block_device_partitions(sdb);
        if(not check_partition_list(partition_table_scan_case)) {
            return false;
        }
        return pass(partition_table_scan_case);
    }

    auto case_multi_sector_read() -> bool
    {
        auto* sda = find_required_device("sda",multi_sector_read_case);
        if(sda == nullptr) {
            return false;
        }
        auto buf = std::vector<char>(multi_sector_read_cnt * sector_size);
        block_read_blocks(sda,multi_sector_read_lba,buf.data(),multi_sector_read_cnt);
        if(not matches_pattern(buf.data(),multi_sector_read_lba,multi_sector_read_cnt,seed_salt)) {
            return fail(multi_sector_read_case,"multi-sector pattern mismatch");
        }
        return pass(multi_sector_read_case);
    }

    auto case_max_transfer_boundary_read() -> bool
    {
        auto* sda = find_required_device("sda",max_transfer_boundary_read_case);
        if(sda == nullptr) {
            return false;
        }
        auto buf = std::vector<char>(max_transfer_boundary_cnt * sector_size);
        block_read_blocks(sda,max_transfer_boundary_lba,buf.data(),max_transfer_boundary_cnt);
        if(not matches_pattern(buf.data(),max_transfer_boundary_lba,max_transfer_boundary_cnt,seed_salt)) {
            return fail(max_transfer_boundary_read_case,"boundary pattern mismatch");
        }
        return pass(max_transfer_boundary_read_case);
    }

    auto case_multi_sector_write() -> bool
    {
        auto* sda = find_required_device("sda",multi_sector_write_case);
        if(sda == nullptr) {
            return false;
        }
        auto backup = std::vector<char>(multi_sector_write_cnt * sector_size);
        auto expected = std::vector<char>(multi_sector_write_cnt * sector_size);
        auto verify = std::vector<char>(multi_sector_write_cnt * sector_size);
        auto restored = std::vector<char>(multi_sector_write_cnt * sector_size);

        block_read_blocks(sda,multi_sector_write_lba,backup.data(),multi_sector_write_cnt);
        fill_pattern(expected.data(),multi_sector_write_lba,multi_sector_write_cnt,write_salt);
        block_write_blocks(sda,multi_sector_write_lba,expected.data(),multi_sector_write_cnt);
        block_read_blocks(sda,multi_sector_write_lba,verify.data(),multi_sector_write_cnt);
        block_write_blocks(sda,multi_sector_write_lba,backup.data(),multi_sector_write_cnt);
        block_read_blocks(sda,multi_sector_write_lba,restored.data(),multi_sector_write_cnt);

        if(not bytes_eq(verify.data(),expected.data(),multi_sector_write_cnt * sector_size)) {
            return fail(multi_sector_write_case,"write/readback mismatch");
        }
        if(not bytes_eq(restored.data(),backup.data(),multi_sector_write_cnt * sector_size)) {
            return fail(multi_sector_write_case,"restore mismatch");
        }
        return pass(multi_sector_write_case);
    }
}

auto disk_regression_mode_requested() -> bool
{
    auto marker = std::array<char,sector_size>{};
    auto* sda = find_block_device("sda");
    disk_regression_case_name = {};
    if(sda == nullptr) {
        disk_regression_mode = false;
        return false;
    }

    block_read_blocks(sda,disk_suite_marker_lba,marker.data(),1);
    disk_regression_mode = bytes_eq(marker.data(),disk_suite_marker,8);
    if(disk_regression_mode) {
        for(auto i = 0u; i + 1 < disk_regression_case_name.size(); ++i) {
            auto const ch = marker[disk_suite_case_offset + i];
            disk_regression_case_name[i] = ch;
            if(ch == '\0') {
                break;
            }
        }
        if(not is_known_case(disk_regression_case_name.data())) {
            disk_regression_case_name = {};
        }
    }
    return disk_regression_mode;
}

auto disk_regression_mode_active() -> bool
{
    return disk_regression_mode;
}

auto disk_regression_mode_case() -> char const*
{
    if(not disk_regression_mode or disk_regression_case_name[0] == '\0') {
        return nullptr;
    }
    return disk_regression_case_name.data();
}

auto run_disk_regression(char const* case_name) -> bool
{
    if(str_eq(case_name,read_sector_case)) {
        return case_read_sector();
    }
    if(str_eq(case_name,cross_sector_case)) {
        return case_cross_sector_read();
    }
    if(str_eq(case_name,read_after_write_case)) {
        return case_read_after_write();
    }
    if(str_eq(case_name,partition_table_scan_case)) {
        return case_partition_table_scan();
    }
    if(str_eq(case_name,multi_sector_read_case)) {
        return case_multi_sector_read();
    }
    if(str_eq(case_name,max_transfer_boundary_read_case)) {
        return case_max_transfer_boundary_read();
    }
    if(str_eq(case_name,multi_sector_write_case)) {
        return case_multi_sector_write();
    }
    return fail(case_name,"unknown case");
}

auto disktest(char const* case_name) -> int
{
    auto const status = intr_get_status();
    intr_enable();
    auto const rc = run_disk_regression(case_name);
    intr_set_status(status);
    return rc;
}
