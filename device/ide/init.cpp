module;

#include <assert.h>
#include <interrupt.h>
#include <string.h>

export module ide.init;

import ide;
import console;
import utility;
import printf;
import lock_guard;
import ide.intr;
import format;
import ide.part;
import algorithm;

export auto ide_init() -> void
{
    console::println("ide_init start");
    auto hd_cnt = *(u8*)(0x475); // 获取硬盘的数量
    ASSERT(hd_cnt > 0);
    channel_cnt = std::div_ceil(hd_cnt,2);

    for(auto i = 0; i != channel_cnt; ++i) {
        auto channel = &channels[i];
        std::sprintf(channel->name,"ide{}",i);
        // 为每个ide通道初始化端口基址及中断向量
        switch(i) {
            case 0: {
                // ide0通道的起始端口号是0x1f0
                channel->port_base = 0x1f0;
                // 从片8295a上倒数第二的中断引脚 硬盘ide0通道的中断向量号
                channel->irq_no = 0x20 + 14;
                break;
            } case 1: {
                channel->port_base = 0x170;
                // 8259a上最后一个中断引脚，用来响应ide1通道上的硬盘中断
                channel->irq_no = 0x20 + 15;
                break;
            } default: {
                PANIC("the channel_cnt must <= 2");
            }
        }
        channel->expecting_intr = false;
        // 不向硬盘写入时，不期待中断
        channel->disk_done.init(0); // 请求数据后会直接阻塞，直到硬盘发中断，唤醒
        register_handler(channel->irq_no,(void*)intr_hd_handler); // 注册中断函数
        // 为channel的两个硬盘数据结构初始化
        for(auto dev_no = 0; dev_no != 2; ++dev_no) {
            auto hd = &channel->devices[dev_no];
            hd->my_channel = channel;
            hd->dev_no = dev_no;
            std::format_to(hd->name,"sd{c}",'a' + i * 2 + dev_no);
            identify_disk(hd);
            if(dev_no != 0) { // dev_no = 0 是内核的裸硬盘不处理，对于带文件系统的副盘 扫描分区处理
                scan_partition(hd,0);
            }
        }
    }

    console::println("\n all partition info");
    partition_list | std::apply[partition_info];
    console::println("ide_init done");

}

