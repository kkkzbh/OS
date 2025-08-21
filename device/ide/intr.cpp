module;

#include <assert.h>
#include <io.h>

export module ide.intr;

import utility;
import ide;

// 硬盘中断处理程序
export extern "C" auto intr_hd_handler(u8 irq_no) -> void
{
    ASSERT(irq_no == 0x2e or irq_no == 0x2f);
    auto ch_no = irq_no - 0x2e;
    auto channel = &channels[ch_no];
    ASSERT(channel->irq_no == irq_no);
    // 不担心该中断是否对应这次的expecting_intr
    // 每次读写硬盘申请锁，保证同步一致性
    if(not channel->expecting_intr) {
        return;
    }
    channel->expecting_intr = false;
    // 释放信号量
    channel->disk_done.release();
    // 再读一次status寄存器的状态，向硬盘控制器表示中断结束的信号
    inb(reg::status(channel));
}