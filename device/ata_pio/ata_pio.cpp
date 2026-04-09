module;

#include <io.h>
#include <time.h>
#include <assert.h>

export module ata.pio;

import utility;
import mutex;
import semaphore;
import array;
import lock_guard;
import format;
import bit;
import console;
import algorithm;
import schedule;
import block.device;
import block.partition;

auto constexpr STAT_ERR = u8(0x01);
auto constexpr STAT_DRQ = u8(0x08);
auto constexpr STAT_DF = u8(0x20);
auto constexpr STAT_BSY = u8(0x80);

auto constexpr DEV_MBS = u8(0xa0);
auto constexpr DEV_LBA = u8(0x40);
auto constexpr DEV_DEV = u8(0x10);

auto constexpr CMD_IDENTIFY = u8(0xec);
auto constexpr CMD_READ_SECTOR = u8(0x20);
auto constexpr CMD_WRITE_SECTOR = u8(0x30);
auto constexpr sector_size = u32(512);
auto constexpr max_transfer = u32(256);
auto constexpr ata_quick_poll_limit = u32(16);
auto constexpr ata_wait_timeout_ticks = u32(500);

auto constexpr max_lba = 80 * 1024 * 1024 / 512 - 1;

export struct ata_pio_device;
export struct ata_channel;

export struct ata_channel_stats
{
    u32 commands_issued{};
    u32 read_sectors{};
    u32 written_sectors{};
    u32 irq_completions{};
    u32 poll_fallbacks{};
    u32 timeouts{};
};

export auto identify_ata_pio_device(ata_pio_device* hd) -> void;
export auto init_channel_runtime(ata_channel& channel) -> void;
export auto reset_ata_channel_stats(ata_channel* channel) -> void;
export auto read_ata_channel_stats(ata_channel const* channel) -> ata_channel_stats;

struct ata_pio_device
{
    block_device base{};
    ata_channel* my_channel;
    u8 dev_no;
    partition prim_parts[4];
    partition logic_parts[8];
};

struct ata_channel
{
    char name[8]{};
    u16 port_base;
    u8 irq_no;
    mutex mtx;
    bool command_active;
    u32 irq_seq;
    u8 last_status;
    u8 last_error;
    semaphore disk_done;
    ata_channel_stats stats{};
    ata_pio_device devices[2];
};

export auto ata_channel_cnt = u8{};
export std::array<ata_channel,2> ata_channels;

namespace reg
{
    export auto data(ata_channel const* channel) -> u16
    {
        return channel->port_base + 0;
    }

    export auto error(ata_channel const* channel) -> u16
    {
        return channel->port_base + 1;
    }

    export auto sect_cnt(ata_channel const* channel) -> u16
    {
        return channel->port_base + 2;
    }

    export auto lba_l(ata_channel const* channel) -> u16
    {
        return channel->port_base + 3;
    }

    export auto lba_m(ata_channel const* channel) -> u16
    {
        return channel->port_base + 4;
    }

    export auto lba_h(ata_channel const* channel) -> u16
    {
        return channel->port_base + 5;
    }

    export auto dev(ata_channel const* channel) -> u16
    {
        return channel->port_base + 6;
    }

    export auto status(ata_channel const* channel) -> u16
    {
        return channel->port_base + 7;
    }

    export auto cmd(ata_channel const* channel) -> u16
    {
        return status(channel);
    }

    export auto alt_status(ata_channel const* channel) -> u16
    {
        return channel->port_base + 0x206;
    }

    export auto ctl(ata_channel const* channel) -> u16
    {
        return alt_status(channel);
    }
}

namespace
{
    enum class ata_wait_kind : u8
    {
        data_ready,
        command_complete,
    };

    enum class ata_io_result : u8
    {
        ok,
        timeout,
        error,
        device_fault,
        unexpected_state,
    };

    auto init_ata_pio_device_runtime(ata_pio_device& hd,ata_channel* channel,u8 dev_no) -> void
    {
        hd.base = {
            .sector_size = sector_size,
            .max_transfer = max_transfer,
        };
        hd.base.read_blocks = nullptr;
        hd.base.write_blocks = nullptr;
        hd.my_channel = channel;
        hd.dev_no = dev_no;
        for(auto& part : hd.prim_parts) {
            init_partition_runtime(part);
        }
        for(auto& part : hd.logic_parts) {
            init_partition_runtime(part);
        }
    }

    auto ata_pio_read_blocks(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void;
    auto ata_pio_write_blocks(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void;

    auto transfer_bytes(u8 sec_cnt) -> u32
    {
        return sec_cnt == 0 ? max_transfer * sector_size : u32(sec_cnt) * sector_size;
    }

    auto encode_sector_count(u32 block_cnt) -> u8
    {
        ASSERT(block_cnt > 0 and block_cnt <= max_transfer);
        return block_cnt == max_transfer ? u8(0) : u8(block_cnt);
    }

    auto as_ata_pio_device(block_device* dev) -> ata_pio_device*
    {
        ASSERT(dev != nullptr);
        return reinterpret_cast<ata_pio_device*>(dev);
    }

    auto ata_400ns_delay(ata_channel* channel) -> void
    {
        inb(reg::alt_status(channel));
        inb(reg::alt_status(channel));
        inb(reg::alt_status(channel));
        inb(reg::alt_status(channel));
    }

    auto read_controller_status(ata_channel* channel,bool clear_irq) -> u8
    {
        auto const status = clear_irq ? inb(reg::status(channel)) : inb(reg::alt_status(channel));
        channel->last_status = status;
        channel->last_error = (status & (STAT_ERR | STAT_DF)) ? inb(reg::error(channel)) : u8(0);
        return status;
    }

    auto classify_status(u8 status,ata_wait_kind kind) -> ata_io_result
    {
        if(status & STAT_ERR) {
            return ata_io_result::error;
        }
        if(status & STAT_DF) {
            return ata_io_result::device_fault;
        }
        if(status & STAT_BSY) {
            return ata_io_result::unexpected_state;
        }
        if(kind == ata_wait_kind::data_ready) {
            return status & STAT_DRQ ? ata_io_result::ok : ata_io_result::unexpected_state;
        }
        return ata_io_result::ok;
    }

    auto finish_timeout_result(ata_channel* channel,ata_wait_kind kind) -> ata_io_result
    {
        auto const status = read_controller_status(channel,true);
        if(auto const result = classify_status(status,kind); result != ata_io_result::unexpected_state) {
            return result;
        }
        return status & STAT_BSY ? ata_io_result::timeout : ata_io_result::unexpected_state;
    }

    auto wait_for_device(ata_pio_device* hd,ata_wait_kind kind) -> ata_io_result
    {
        auto channel = hd->my_channel;
        for(auto spin = 0u; spin != ata_quick_poll_limit; ++spin) {
            auto const status = read_controller_status(channel,false);
            if(auto const result = classify_status(status,kind); result != ata_io_result::unexpected_state) {
                return result;
            }
        }

        auto const start_tick = ticks;
        auto used_poll_fallback = false;
        while(ticks - start_tick < ata_wait_timeout_ticks) {
            if(channel->disk_done.try_acquire()) {
                auto const status = read_controller_status(channel,true);
                if(auto const result = classify_status(status,kind); result != ata_io_result::unexpected_state) {
                    return result;
                }
                continue;
            }

            if(not used_poll_fallback) {
                ++channel->stats.poll_fallbacks;
                used_poll_fallback = true;
            }

            auto const status = read_controller_status(channel,false);
            if(auto const result = classify_status(status,kind); result != ata_io_result::unexpected_state) {
                return result;
            }
            thread_yield();
        }

        ++channel->stats.timeouts;
        return finish_timeout_result(channel,kind);
    }

    auto drain_completion_queue(ata_channel* channel) -> void
    {
        while(channel->disk_done.try_acquire()) {
        }
    }

    auto command_result_name(ata_io_result result) -> char const*
    {
        switch(result) {
            case ata_io_result::ok:
                return "ok";
            case ata_io_result::timeout:
                return "timeout";
            case ata_io_result::error:
                return "error";
            case ata_io_result::device_fault:
                return "device_fault";
            case ata_io_result::unexpected_state:
                return "unexpected_state";
        }
        return "unknown";
    }

    auto panic_io_failure(
        ata_pio_device* hd,
        char const* op,
        u32 lba,
        u32 chunk_lba,
        u32 chunk_cnt,
        ata_io_result result
    ) -> void
    {
        auto error = std::array<char,160>{};
        std::format_to(
            error.data(),
            "{} {} failed: lba={} chunk_lba={} chunk_cnt={} status=0x{:x} error=0x{:x} result={}\n",
            hd->base.name,
            op,
            lba,
            chunk_lba,
            chunk_cnt,
            u32(hd->my_channel->last_status),
            u32(hd->my_channel->last_error),
            command_result_name(result)
        );
        PANIC(error.data());
    }

    auto finish_command(ata_channel* channel) -> void
    {
        channel->command_active = false;
    }

    auto select_disk(ata_pio_device* hd) -> void
    {
        auto device = DEV_MBS | DEV_LBA;
        if(hd->dev_no == 1) {
            device |= DEV_DEV;
        }
        outb(reg::dev(hd->my_channel),device);
        ata_400ns_delay(hd->my_channel);
    }

    auto select_sector(ata_pio_device* hd,u32 lba,u8 sec_cnt) -> void
    {
        ASSERT(lba <= max_lba);
        auto channel = hd->my_channel;
        outb(reg::sect_cnt(channel),sec_cnt);
        outb(reg::lba_l(channel),lba);
        outb(reg::lba_m(channel),lba >> 8);
        outb(reg::lba_h(channel),lba >> 16);

        auto device = DEV_MBS | DEV_LBA | (lba >> 24);
        if(hd->dev_no == 1) {
            device |= DEV_DEV;
        }
        outb(reg::dev(channel),device);
        ata_400ns_delay(channel);
    }

    auto cmd_out(ata_channel* channel,u8 cmd) -> void
    {
        drain_completion_queue(channel);
        channel->command_active = true;
        channel->last_status = 0;
        channel->last_error = 0;
        ++channel->stats.commands_issued;
        outb(reg::cmd(channel),cmd);
        ata_400ns_delay(channel);
    }

    auto read_from_sector(ata_pio_device* hd,void* buf,u8 sec_cnt) -> void
    {
        insw(reg::data(hd->my_channel),buf,transfer_bytes(sec_cnt) / 2);
    }

    auto write_sector(ata_pio_device* hd,void* buf,u8 sec_cnt) -> void
    {
        outsw(reg::data(hd->my_channel),buf,transfer_bytes(sec_cnt) / 2);
    }

    auto pio_read_chunk(ata_pio_device* hd,u32 lba,u8* buf,u32 block_cnt) -> ata_io_result
    {
        auto const hw_block_cnt = encode_sector_count(block_cnt);
        select_sector(hd,lba,hw_block_cnt);
        cmd_out(hd->my_channel,CMD_READ_SECTOR);

        auto transferred = 0u;
        auto result = ata_io_result::ok;
        while(transferred != block_cnt) {
            result = wait_for_device(hd,ata_wait_kind::data_ready);
            if(result != ata_io_result::ok) {
                break;
            }
            read_from_sector(hd,buf + transferred * sector_size,1);
            ++transferred;
        }

        hd->my_channel->stats.read_sectors += transferred;
        finish_command(hd->my_channel);
        return result;
    }

    auto pio_write_chunk(ata_pio_device* hd,u32 lba,u8* buf,u32 block_cnt) -> ata_io_result
    {
        auto const hw_block_cnt = encode_sector_count(block_cnt);
        select_sector(hd,lba,hw_block_cnt);
        cmd_out(hd->my_channel,CMD_WRITE_SECTOR);

        auto transferred = 0u;
        auto result = ata_io_result::ok;
        while(transferred != block_cnt) {
            result = wait_for_device(hd,ata_wait_kind::data_ready);
            if(result != ata_io_result::ok) {
                break;
            }
            write_sector(hd,buf + transferred * sector_size,1);
            ++transferred;
        }

        hd->my_channel->stats.written_sectors += transferred;
        if(result == ata_io_result::ok) {
            result = wait_for_device(hd,ata_wait_kind::command_complete);
        }
        finish_command(hd->my_channel);
        return result;
    }
}

auto init_channel_runtime(ata_channel& channel) -> void
{
    channel.mtx.init();
    channel.command_active = false;
    channel.irq_seq = 0;
    channel.last_status = 0;
    channel.last_error = 0;
    channel.disk_done.init(0);
    channel.stats = {};
    for(auto dev_no = 0; dev_no != 2; ++dev_no) {
        init_ata_pio_device_runtime(channel.devices[dev_no],&channel,dev_no);
        channel.devices[dev_no].base.read_blocks = ata_pio_read_blocks;
        channel.devices[dev_no].base.write_blocks = ata_pio_write_blocks;
    }
}

auto reset_ata_channel_stats(ata_channel* channel) -> void
{
    ASSERT(channel != nullptr);
    channel->stats = {};
}

auto read_ata_channel_stats(ata_channel const* channel) -> ata_channel_stats
{
    ASSERT(channel != nullptr);
    return channel->stats;
}

namespace
{
    auto ata_pio_read_blocks(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void
    {
        auto hd = as_ata_pio_device(dev);
        ASSERT(buf != nullptr);
        ASSERT(block_cnt > 0);
        ASSERT(lba <= max_lba and block_cnt - 1 <= max_lba - lba);

        auto lcg = lock_guard{ hd->my_channel->mtx };
        select_disk(hd);

        auto* bytes = reinterpret_cast<u8*>(buf);
        for(auto done = 0u; done < block_cnt;) {
            auto const chunk_cnt = std::min(block_cnt - done,max_transfer);
            auto const chunk_lba = lba + done;
            if(auto const result = pio_read_chunk(hd,chunk_lba,bytes + done * sector_size,chunk_cnt);
                result != ata_io_result::ok) {
                panic_io_failure(hd,"read",lba,chunk_lba,chunk_cnt,result);
            }
            done += chunk_cnt;
        }
    }

    auto ata_pio_write_blocks(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void
    {
        auto hd = as_ata_pio_device(dev);
        ASSERT(buf != nullptr);
        ASSERT(block_cnt > 0);
        ASSERT(lba <= max_lba and block_cnt - 1 <= max_lba - lba);

        auto lcg = lock_guard{ hd->my_channel->mtx };
        select_disk(hd);

        auto* bytes = reinterpret_cast<u8*>(buf);
        for(auto done = 0u; done < block_cnt;) {
            auto const chunk_cnt = std::min(block_cnt - done,max_transfer);
            auto const chunk_lba = lba + done;
            if(auto const result = pio_write_chunk(hd,chunk_lba,bytes + done * sector_size,chunk_cnt);
                result != ata_io_result::ok) {
                panic_io_failure(hd,"write",lba,chunk_lba,chunk_cnt,result);
            }
            done += chunk_cnt;
        }
    }
}

auto identify_ata_pio_device(ata_pio_device* hd) -> void
{
    auto id_info = std::array<char,512>{};
    select_disk(hd);
    cmd_out(hd->my_channel,CMD_IDENTIFY);
    auto const result = wait_for_device(hd,ata_wait_kind::data_ready);
    if(result != ata_io_result::ok) {
        finish_command(hd->my_channel);
        panic_io_failure(hd,"identify",0,0,1,result);
    }
    read_from_sector(hd,id_info.data(),1);
    hd->my_channel->stats.read_sectors += 1;
    finish_command(hd->my_channel);

    auto buf = std::array<char,64>{};
    auto constexpr sn_start = u8(10 * 2);
    auto constexpr sn_len = u8(20);
    auto constexpr md_start = u8(27 * 2);
    auto constexpr md_len = u8(40);
    std::pair_byte_swap(&id_info[sn_start],buf.data(),sn_len);
    console::println("  disk {} info:\n     SN: {}",hd->base.name,buf.data());
    buf | std::fill[0];
    std::pair_byte_swap(&id_info[md_start],buf.data(),md_len);
    console::println("      MODULE: {}",buf.data());
    auto sectors = *((u32*)&id_info[60 * 2]);
    console::println("      SECTORS: {}",sectors);
    console::println("      CAPACITY: {}MB",sectors * 512 / 1024 / 1024);
}
