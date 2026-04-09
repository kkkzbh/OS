module;

#include <assert.h>
#include <io.h>

export module ata.pio.intr;

import utility;
import ata.pio;

namespace
{
    auto constexpr stat_err = u8(0x01);
    auto constexpr stat_df = u8(0x20);
}

export extern "C" auto intr_hd_handler(u8 irq_no) -> void
{
    ASSERT(irq_no == 0x2e or irq_no == 0x2f);
    auto ch_no = irq_no - 0x2e;
    auto channel = &ata_channels[ch_no];
    ASSERT(channel->irq_no == irq_no);
    auto const status = inb(reg::status(channel));
    channel->last_status = status;
    channel->last_error = (status & (stat_err | stat_df)) ? inb(reg::error(channel)) : u8(0);
    if(not channel->command_active) {
        return;
    }
    ++channel->irq_seq;
    ++channel->stats.irq_completions;
    channel->disk_done.release();
}
