module;

#include <assert.h>
#include <interrupt.h>
#include <string.h>

export module ata.pio.init;

import ata.pio;
import block.device;
import block.partition;
import console;
import utility;
import format;
import lock_guard;
import ata.pio.intr;
import algorithm;

export auto ata_pio_init() -> void
{
    console::println("ata_pio_init start");
    auto hd_cnt = *(u8*)(0x475);
    ASSERT(hd_cnt > 0);
    ata_channel_cnt = std::div_ceil(hd_cnt,2);
    reset_block_device_registry();
    partition_runtime_init();

    for(auto i = 0; i != ata_channel_cnt; ++i) {
        auto channel = &ata_channels[i];
        init_channel_runtime(*channel);
        std::format_to(channel->name,"ata{}",i);
        switch(i) {
            case 0: {
                channel->port_base = 0x1f0;
                channel->irq_no = 0x20 + 14;
                break;
            } case 1: {
                channel->port_base = 0x170;
                channel->irq_no = 0x20 + 15;
                break;
            } default: {
                PANIC("the ata_channel_cnt must <= 2");
            }
        }
        register_handler(channel->irq_no,(void*)intr_hd_handler);
        for(auto dev_no = 0; dev_no != 2; ++dev_no) {
            auto hd = &channel->devices[dev_no];
            std::format_to(hd->base.name,"sd{c}",'a' + i * 2 + dev_no);
            identify_ata_pio_device(hd);
            register_block_device(&hd->base);
            if(dev_no != 0) {
                scan_block_device_partitions(&hd->base);
            }
        }
    }

    console::println("\n all partition info");
    partition_list | std::apply[partition_info];
    console::println("ata_pio_init done");
}
