module;

#include <io.h>
#include <interrupt.h>

export module ata.pio.regression;

import ata.pio;
import block.device;
import block.partition;
import console;
import array;
import format;
import sleep;
import utility;
import vector;

namespace
{
    auto constexpr sector_size = u32(512);
    auto constexpr stat_bsy = u8(0x80);
    auto constexpr stat_drq = u8(0x08);
    auto constexpr dev_mbs = u8(0xa0);
    auto constexpr dev_lba = u8(0x40);
    auto constexpr dev_dev = u8(0x10);
    auto constexpr cmd_read_sector = u8(0x20);
    auto constexpr cmd_write_sector = u8(0x30);

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

    auto str_eq(char const* lhs, char const* rhs) -> bool
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

    auto bytes_eq(char const* lhs, char const* rhs, u32 count) -> bool
    {
        for(auto i = 0u; i != count; ++i) {
            if(lhs[i] != rhs[i]) {
                return false;
            }
        }
        return true;
    }

    auto pattern_byte(u32 lba, u32 byte_off, u8 salt) -> char
    {
        return char((u32(salt) + ((lba * 17) & 0xff) + byte_off) & 0xff);
    }

    auto raw_select_disk(ata_pio_device* hd) -> void
    {
        auto device = u8(dev_mbs | dev_lba);
        if(hd->dev_no == 1) {
            device |= dev_dev;
        }
        outb(reg::dev(hd->my_channel), device);
    }

    auto raw_select_sector(ata_pio_device* hd, u32 lba, u8 sec_cnt) -> void
    {
        auto* channel = hd->my_channel;
        outb(reg::sect_cnt(channel), sec_cnt);
        outb(reg::lba_l(channel), lba);
        outb(reg::lba_m(channel), lba >> 8);
        outb(reg::lba_h(channel), lba >> 16);

        auto device = u8(dev_mbs | dev_lba | (lba >> 24));
        if(hd->dev_no == 1) {
            device |= dev_dev;
        }
        outb(reg::dev(channel), device);
    }

    auto raw_wait_for_data_request(ata_pio_device* hd) -> bool
    {
        auto* channel = hd->my_channel;
        auto wait = i32(30s);
        while(wait > 0) {
            auto const status = inb(reg::status(channel));
            if(not (status & stat_bsy)) {
                return status & stat_drq;
            }
            sleep(10ms);
            wait -= 10;
        }
        return false;
    }

    auto raw_wait_for_not_busy(ata_pio_device* hd) -> bool
    {
        auto* channel = hd->my_channel;
        auto wait = i32(30s);
        while(wait > 0) {
            if(not (inb(reg::status(channel)) & stat_bsy)) {
                return true;
            }
            sleep(10ms);
            wait -= 10;
        }
        return false;
    }

    auto raw_transfer_bytes(u8 sec_cnt) -> u32
    {
        return sec_cnt == 0 ? 256 * sector_size : u32(sec_cnt) * sector_size;
    }

    auto raw_read_words(ata_pio_device* hd, void* buf, u8 sec_cnt) -> void
    {
        insw(reg::data(hd->my_channel), buf, raw_transfer_bytes(sec_cnt) / 2);
    }

    auto raw_write_words(ata_pio_device* hd, void* buf, u8 sec_cnt) -> void
    {
        outsw(reg::data(hd->my_channel), buf, raw_transfer_bytes(sec_cnt) / 2);
    }

    auto raw_encode_sector_count(u32 sec_cnt) -> u8
    {
        return sec_cnt == 256 ? u8(0) : u8(sec_cnt);
    }

    auto raw_pio_read(ata_pio_device* hd, u32 lba, void* buf, u32 sec_cnt) -> bool
    {
        raw_select_disk(hd);
        for(auto done = 0u; done != sec_cnt;) {
            auto const chunk_secs = std::min(sec_cnt - done, 256u);
            auto const hw_secs = raw_encode_sector_count(chunk_secs);
            raw_select_sector(hd, lba + done, hw_secs);
            outb(reg::cmd(hd->my_channel), cmd_read_sector);
            for(auto sec = 0u; sec != chunk_secs; ++sec) {
                if(not raw_wait_for_data_request(hd)) {
                    return false;
                }
                raw_read_words(hd, (void*)((u32)buf + (done + sec) * sector_size), 1);
            }
            done += chunk_secs;
        }
        return true;
    }

    auto raw_pio_write(ata_pio_device* hd, u32 lba, void* buf, u32 sec_cnt) -> bool
    {
        raw_select_disk(hd);
        for(auto done = 0u; done != sec_cnt;) {
            auto const chunk_secs = std::min(sec_cnt - done, 256u);
            auto const hw_secs = raw_encode_sector_count(chunk_secs);
            raw_select_sector(hd, lba + done, hw_secs);
            outb(reg::cmd(hd->my_channel), cmd_write_sector);
            for(auto sec = 0u; sec != chunk_secs; ++sec) {
                if(not raw_wait_for_data_request(hd)) {
                    return false;
                }
                raw_write_words(hd, (void*)((u32)buf + (done + sec) * sector_size), 1);
            }
            if(not raw_wait_for_not_busy(hd)) {
                return false;
            }
            done += chunk_secs;
        }
        return true;
    }

    auto fill_pattern(char* buf, u32 start_lba, u32 sec_cnt, u8 salt) -> void
    {
        for(auto sec = 0u; sec != sec_cnt; ++sec) {
            for(auto byte_off = 0u; byte_off != sector_size; ++byte_off) {
                buf[sec * sector_size + byte_off] = pattern_byte(start_lba + sec, byte_off, salt);
            }
        }
    }

    auto matches_pattern(char const* buf, u32 start_lba, u32 sec_cnt, u8 salt) -> bool
    {
        for(auto sec = 0u; sec != sec_cnt; ++sec) {
            for(auto byte_off = 0u; byte_off != sector_size; ++byte_off) {
                if(buf[sec * sector_size + byte_off] != pattern_byte(start_lba + sec, byte_off, salt)) {
                    return false;
                }
            }
        }
        return true;
    }

    auto is_known_case(char const* case_name) -> bool
    {
        return str_eq(case_name, read_sector_case)
            or str_eq(case_name, cross_sector_case)
            or str_eq(case_name, read_after_write_case)
            or str_eq(case_name, partition_table_scan_case)
            or str_eq(case_name, multi_sector_read_case)
            or str_eq(case_name, max_transfer_boundary_read_case)
            or str_eq(case_name, multi_sector_write_case);
    }

    auto fail(char const* case_name, char const* reason) -> bool
    {
        console::println("DISKERR:{}:{}", case_name, reason);
        console::println("DISKCASE:{}:FAIL", case_name);
        return false;
    }

    auto pass(char const* case_name) -> bool
    {
        console::println("DISKCASE:{}:PASS", case_name);
        return true;
    }

    auto fail_stat_mismatch(char const* case_name, char const* stat_name, u32 expected, u32 actual) -> bool
    {
        auto reason = std::array<char, 96>{};
        std::format_to(reason.data(), "{} expected {} got {}", stat_name, expected, actual);
        return fail(case_name, reason.data());
    }

    auto print_driver_stats(char const* case_name, ata_channel_stats const& stats) -> void
    {
        console::println(
            "DISKSTAT:{}:cmds={}:irqs={}:timeouts={}:reads={}:writes={}",
            case_name,
            stats.commands_issued,
            stats.irq_completions,
            stats.timeouts,
            stats.read_sectors,
            stats.written_sectors
        );
    }

    auto expect_driver_stats(
        char const* case_name,
        ata_channel_stats const& stats,
        u32 expected_cmds,
        u32 expected_timeouts,
        u32 expected_reads,
        u32 expected_writes
    ) -> bool
    {
        if(stats.commands_issued != expected_cmds) {
            return fail_stat_mismatch(case_name, "commands", expected_cmds, stats.commands_issued);
        }
        if(stats.timeouts != expected_timeouts) {
            return fail_stat_mismatch(case_name, "timeouts", expected_timeouts, stats.timeouts);
        }
        if(stats.read_sectors != expected_reads) {
            return fail_stat_mismatch(case_name, "read_sectors", expected_reads, stats.read_sectors);
        }
        if(stats.written_sectors != expected_writes) {
            return fail_stat_mismatch(case_name, "written_sectors", expected_writes, stats.written_sectors);
        }
        return true;
    }

    auto check_partition(partition const& part, char const* name, u32 start_lba, u32 sec_cnt) -> bool
    {
        return str_eq(part.name, name) and part.start_lba == start_lba and part.sec_cnt == sec_cnt and part.device != nullptr;
    }

    auto check_empty_partition(partition const& part) -> bool
    {
        return part.start_lba == 0 and part.sec_cnt == 0 and part.name[0] == '\0';
    }

    auto case_read_sector() -> bool
    {
        auto* sda = &ata_channels[0].devices[0];
        auto buf = std::array<char, sector_size>{};
        if(not raw_pio_read(sda, read_sector_lba, buf.data(), 1)) {
            return fail(read_sector_case, "raw read failed");
        }
        if(not matches_pattern(buf.data(), read_sector_lba, 1, seed_salt)) {
            return fail(read_sector_case, "sector pattern mismatch");
        }
        return pass(read_sector_case);
    }

    auto case_cross_sector_read() -> bool
    {
        auto* sda = &ata_channels[0].devices[0];
        auto buf = std::array<char, sector_size * 2>{};
        if(not raw_pio_read(sda, cross_sector_lba, buf.data(), 2)) {
            return fail(cross_sector_case, "raw read failed");
        }
        if(not matches_pattern(buf.data(), cross_sector_lba, 2, seed_salt)) {
            return fail(cross_sector_case, "cross-sector pattern mismatch");
        }
        return pass(cross_sector_case);
    }

    auto case_read_after_write() -> bool
    {
        auto* sda = &ata_channels[0].devices[0];
        auto backup = std::array<char, sector_size>{};
        auto expected = std::array<char, sector_size>{};
        auto verify = std::array<char, sector_size>{};
        auto restored = std::array<char, sector_size>{};

        if(not raw_pio_read(sda, scratch_sector_lba, backup.data(), 1)) {
            return fail(read_after_write_case, "backup read failed");
        }
        fill_pattern(expected.data(), scratch_sector_lba, 1, write_salt);

        if(not raw_pio_write(sda, scratch_sector_lba, expected.data(), 1)) {
            return fail(read_after_write_case, "test write failed");
        }
        if(not raw_pio_read(sda, scratch_sector_lba, verify.data(), 1)) {
            return fail(read_after_write_case, "verify read failed");
        }
        if(not raw_pio_write(sda, scratch_sector_lba, backup.data(), 1)) {
            return fail(read_after_write_case, "restore write failed");
        }
        if(not raw_pio_read(sda, scratch_sector_lba, restored.data(), 1)) {
            return fail(read_after_write_case, "restore read failed");
        }

        if(not bytes_eq(verify.data(), expected.data(), sector_size)) {
            return fail(read_after_write_case, "write/readback mismatch");
        }
        if(not bytes_eq(restored.data(), backup.data(), sector_size)) {
            return fail(read_after_write_case, "restore mismatch");
        }
        return pass(read_after_write_case);
    }

    auto case_partition_table_scan() -> bool
    {
        auto* sdb = &ata_channels[0].devices[1];
        auto* table = find_block_partition_table(&sdb->base);
        if(table == nullptr) {
            return fail(partition_table_scan_case, "partition table missing");
        }
        if(not check_partition(table->primary[0], "sdb1", 0x800, 0x8000)) {
            return fail(partition_table_scan_case, "sdb1 mismatch");
        }
        if(not check_empty_partition(table->primary[1])) {
            return fail(partition_table_scan_case, "primary slot 2 should be empty");
        }
        if(not check_empty_partition(table->primary[2])) {
            return fail(partition_table_scan_case, "primary slot 3 should be empty");
        }
        if(not check_empty_partition(table->primary[3])) {
            return fail(partition_table_scan_case, "primary slot 4 should be empty");
        }

        if(not check_partition(table->logical[0], "sdb5", 0x9000, 0x4800)) {
            return fail(partition_table_scan_case, "sdb5 mismatch");
        }
        if(not check_partition(table->logical[1], "sdb6", 0xE000, 0x6000)) {
            return fail(partition_table_scan_case, "sdb6 mismatch");
        }
        if(not check_partition(table->logical[2], "sdb7", 0x14800, 0x3800)) {
            return fail(partition_table_scan_case, "sdb7 mismatch");
        }
        if(not check_partition(table->logical[3], "sdb8", 0x18800, 0x7800)) {
            return fail(partition_table_scan_case, "sdb8 mismatch");
        }
        if(not check_partition(table->logical[4], "sdb9", 0x20800, 0x75E0)) {
            return fail(partition_table_scan_case, "sdb9 mismatch");
        }
        for(auto idx = 5u; idx != 8; ++idx) {
            if(not check_empty_partition(table->logical[idx])) {
                return fail(partition_table_scan_case, "unused logical partition slot should be empty");
            }
        }
        return pass(partition_table_scan_case);
    }

    auto case_multi_sector_read() -> bool
    {
        auto* sda = &ata_channels[0].devices[0];
        auto buf = std::vector<char>(multi_sector_read_cnt * sector_size);
        reset_ata_channel_stats(&ata_channels[0]);
        block_read_blocks(&sda->base, multi_sector_read_lba, buf.data(), multi_sector_read_cnt);
        auto const stats = read_ata_channel_stats(&ata_channels[0]);
        print_driver_stats(multi_sector_read_case, stats);
        if(not matches_pattern(buf.data(), multi_sector_read_lba, multi_sector_read_cnt, seed_salt)) {
            return fail(multi_sector_read_case, "multi-sector pattern mismatch");
        }
        if(not expect_driver_stats(multi_sector_read_case, stats, 1, 0, multi_sector_read_cnt, 0)) {
            return false;
        }
        return pass(multi_sector_read_case);
    }

    auto case_max_transfer_boundary_read() -> bool
    {
        auto* sda = &ata_channels[0].devices[0];
        auto buf = std::vector<char>(max_transfer_boundary_cnt * sector_size);
        reset_ata_channel_stats(&ata_channels[0]);
        block_read_blocks(&sda->base, max_transfer_boundary_lba, buf.data(), max_transfer_boundary_cnt);
        auto const stats = read_ata_channel_stats(&ata_channels[0]);
        print_driver_stats(max_transfer_boundary_read_case, stats);
        if(not matches_pattern(buf.data(), max_transfer_boundary_lba, max_transfer_boundary_cnt, seed_salt)) {
            return fail(max_transfer_boundary_read_case, "boundary pattern mismatch");
        }
        if(not expect_driver_stats(max_transfer_boundary_read_case, stats, 2, 0, max_transfer_boundary_cnt, 0)) {
            return false;
        }
        return pass(max_transfer_boundary_read_case);
    }

    auto case_multi_sector_write() -> bool
    {
        auto* sda = &ata_channels[0].devices[0];
        auto backup = std::vector<char>(multi_sector_write_cnt * sector_size);
        auto expected = std::vector<char>(multi_sector_write_cnt * sector_size);
        auto verify = std::vector<char>(multi_sector_write_cnt * sector_size);
        auto restored = std::vector<char>(multi_sector_write_cnt * sector_size);

        if(not raw_pio_read(sda, multi_sector_write_lba, backup.data(), multi_sector_write_cnt)) {
            return fail(multi_sector_write_case, "backup read failed");
        }
        fill_pattern(expected.data(), multi_sector_write_lba, multi_sector_write_cnt, write_salt);

        reset_ata_channel_stats(&ata_channels[0]);
        block_write_blocks(&sda->base, multi_sector_write_lba, expected.data(), multi_sector_write_cnt);
        block_read_blocks(&sda->base, multi_sector_write_lba, verify.data(), multi_sector_write_cnt);
        auto const stats = read_ata_channel_stats(&ata_channels[0]);
        print_driver_stats(multi_sector_write_case, stats);

        if(not raw_pio_write(sda, multi_sector_write_lba, backup.data(), multi_sector_write_cnt)) {
            return fail(multi_sector_write_case, "restore write failed");
        }
        if(not raw_pio_read(sda, multi_sector_write_lba, restored.data(), multi_sector_write_cnt)) {
            return fail(multi_sector_write_case, "restore read failed");
        }

        if(not bytes_eq(verify.data(), expected.data(), multi_sector_write_cnt * sector_size)) {
            return fail(multi_sector_write_case, "write/readback mismatch");
        }
        if(not bytes_eq(restored.data(), backup.data(), multi_sector_write_cnt * sector_size)) {
            return fail(multi_sector_write_case, "restore mismatch");
        }
        if(not expect_driver_stats(multi_sector_write_case, stats, 2, 0, multi_sector_write_cnt, multi_sector_write_cnt)) {
            return false;
        }
        return pass(multi_sector_write_case);
    }
}

export auto disk_regression_mode_requested() -> bool
{
    auto marker = std::array<char, sector_size>{};
    auto* sda = &ata_channels[0].devices[0];
    disk_regression_case_name = {};
    if(not raw_pio_read(sda, disk_suite_marker_lba, marker.data(), 1)) {
        disk_regression_mode = false;
        return false;
    }
    disk_regression_mode = bytes_eq(marker.data(), disk_suite_marker, 8);
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

export auto disk_regression_mode_active() -> bool
{
    return disk_regression_mode;
}

export auto disk_regression_mode_case() -> char const*
{
    if(not disk_regression_mode or disk_regression_case_name[0] == '\0') {
        return nullptr;
    }
    return disk_regression_case_name.data();
}

export auto is_disk_regression_case(char const* case_name) -> bool
{
    return is_known_case(case_name);
}

export auto run_disk_regression(char const* case_name) -> bool
{
    if(str_eq(case_name, read_sector_case)) {
        return case_read_sector();
    }
    if(str_eq(case_name, cross_sector_case)) {
        return case_cross_sector_read();
    }
    if(str_eq(case_name, read_after_write_case)) {
        return case_read_after_write();
    }
    if(str_eq(case_name, partition_table_scan_case)) {
        return case_partition_table_scan();
    }
    if(str_eq(case_name, multi_sector_read_case)) {
        return case_multi_sector_read();
    }
    if(str_eq(case_name, max_transfer_boundary_read_case)) {
        return case_max_transfer_boundary_read();
    }
    if(str_eq(case_name, multi_sector_write_case)) {
        return case_multi_sector_write();
    }
    return fail(case_name, "unknown case");
}

export auto disktest(char const* case_name) -> int
{
    auto const status = intr_get_status();
    intr_enable();
    auto const rc = run_disk_regression(case_name);
    intr_set_status(status);
    return rc;
}
