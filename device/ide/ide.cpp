module;

#include <io.h>
#include <assert.h>

export module ide;

import utility;
import list;
import bitmap;
import mutex;
import semaphore;
import array;
import sleep;
import lock_guard;
import printf;
import bit;
import console;
import algorithm;

struct super_block
{
    u32 magic;              // 魔数标识文件系统类型(适用于多文件系统)
    u32 sec_cnt;            // 本分区总共的扇区数
    u32 inode_cnt;          // 本分区inode数量
    u32 lba_base;           // 本分区的起始lba地址

    u32 block_bitmap_lba;   // 块位图的起始扇区地址
    u32 block_bitmap_sects; // 扇区位图占用的扇区地址 (为方便，定义1块 == 1扇区)

    u32 inode_bitmap_lba;   // inode位图的起始扇区lba地址
    u32 inode_bitmap_sects; // inode位图占的扇区数量

    u32 inode_table_lba;    // inode表的起始lba地址
    u32 inode_table_sects;  // inode表占用的扇区数量

    u32 data_start_lba;     // 数据区开始的第一个扇区号
    u32 root_inode_no;      // 根目录所在的inode号
    u32 dir_entry_size;     // 目录项大小

    std::array<u8,460> pad; // 凑够512字节一个扇区

};

// reg::alt_status寄存器的一些关键位
auto constexpr ALT_STAT_BSY = 0x80;     // 硬盘忙
auto constexpr ALT_STAT_DRAY = 0x40;    // 驱动器准备好
auto constexpr ALT_STAT_DRQ = 0x8;      // 数据传输准备好了

auto constexpr STAT_BSY = 0x80;     // 硬盘忙
auto constexpr STAT_DRAY = 0x40;    // 驱动器准备好
auto constexpr STAT_DRQ = 0x8;      // 数据传输准备好了

// device寄存器的一些关键位
auto constexpr DEV_MBS = 0xa0;          // 第7位, 5位固定为1
auto constexpr DEV_LBA = 0x40;
auto constexpr DEV_DEV = 0x10;

// 一些硬盘操作的指令
auto constexpr CMD_IDENTIFY = 0xec;     // identify指令
auto constexpr CMD_READ_SECTOR = 0x20;  // 读扇区指令
auto constexpr CMD_WRITE_SECTOR = 0x30; // 写扇区指令

// 定义可读写的最大扇区数，调试用
auto constexpr max_lba = 80 * 1024 * 1024 / 512 - 1;   // 仅80MB硬盘

export struct disk;
export struct ide_channel;

export auto ide_read(disk* hd,u32 lba,void* buf,u32 sec_cnt) -> void;

export auto ide_write(disk* hd,u32 lba,void* buf,u32 sec_cnt) -> void;

export auto identify_disk(disk* hd) -> void;

// 磁盘分区
export struct partition
{
    u32 start_lba;              // 起始扇区
    u32 sec_cnt;                // 扇区数
    disk* my_disk;              // 分区所属的硬盘
    list::node part_tag;        // 于队列的标记
    char name[8];               // 分区名称
    super_block* sb;            // 本分区的超级块
    bitmap block;               // 块位图
    bitmap inode;               // i结点位图
    list open_inodes;           // 本分区打开的i结点队列
};

// 硬盘结构
struct disk
{
    char name[8];               // 磁盘名称
    ide_channel* my_channel;    // 磁盘属于哪儿个ide通道
    u8 dev_no;                  // 是主0 还是从1
    partition prim_parts[4];    // 最多4个主分区
    partition logic_parts[8];   // 设置支持8个逻辑分区
};

// ata通道结构
struct ide_channel
{
    char name[8];               // 本ata通道名称
    u16 port_base;              // 本通道的起始端口号
    u8 irq_no;                  // 本通道所用的中断号
    mutex mtx;                  // 通道锁
    bool expecting_intr;        // 表示等待硬盘的中断
    semaphore disk_done;        // 用于阻塞，唤醒驱动程序
    disk devices[2];            // 一个通道上连接两个硬盘，一主一从
};

export auto channel_cnt = u8{};        // 按硬盘数计算的通道数
export auto channels = std::array<ide_channel,2>{};

namespace reg
{
    export auto data(ide_channel const* idec) -> u16
    {
        return idec->port_base + 0;
    }

    export auto error(ide_channel const* idec) -> u16
    {
        return idec->port_base + 1;
    }

    export auto sect_cnt(ide_channel const* idec) -> u16
    {
        return idec->port_base + 2;
    }

    export auto lba_l(ide_channel const* idec) -> u16
    {
        return idec->port_base + 3;
    }

    export auto lba_m(ide_channel const* idec) -> u16
    {
        return idec->port_base + 4;
    }

    export auto lba_h(ide_channel const* idec) -> u16
    {
        return idec->port_base + 5;
    }

    export auto dev(ide_channel const* idec) -> u16
    {
        return idec->port_base + 6;
    }

    export auto status(ide_channel const* idec) -> u16
    {
        return idec->port_base + 7;
    }

    export auto cmd(ide_channel const* idec) -> u16
    {
        return status(idec);
    }

    export auto alt_status(ide_channel const* idec) -> u16
    {
        return idec->port_base + 0x206;
    }

    export auto ctl(ide_channel const* idec) -> u16
    {
        return alt_status(idec);
    }

}

// 选择读写的硬盘
auto select_disk(disk* hd) -> void
{
    auto device = DEV_MBS | DEV_LBA;
    if(hd->dev_no == 1) { // 如果是从盘 DEV位置1
        device |= DEV_DEV;
    }
    outb(reg::dev(hd->my_channel),device);
}

// 向硬盘控制器写入起始扇区地址以及要读写的扇区数
auto select_sector(disk* hd,u32 lba,u8 sec_cnt) -> void
{
    ASSERT(lba <= max_lba);
    auto channel = hd->my_channel;
    // 写入要读写的扇区数
    outb(reg::sect_cnt(channel),sec_cnt); //如果cnt = 0表示读写256个扇区

    // 写入lba地址，即扇区号
    outb(reg::lba_l(channel),lba);
    outb(reg::lba_m(channel),lba >> 8);
    outb(reg::lba_h(channel),lba >> 16);

    // lba的24~27位要存在device寄存器的0~3位，遂重新写入device寄存器
    auto device = DEV_MBS | DEV_LBA | (lba >> 24);
    if(hd->dev_no == 1) {
        device |= DEV_DEV;
    }
    outb(reg::dev(channel),device);
}

// 向通道channel发命令cmd
auto cmd_out(ide_channel* channel,u8 cmd) -> void
{
    // 发命令则置true，硬盘中断处理程序会根据该值判断
    channel->expecting_intr = true;
    outb(reg::cmd(channel),cmd);
}

// 硬盘读入sec_cnt个扇区的数据到buf
auto read_from_sector(disk* hd,void* buf,u8 sec_cnt) -> void
{
    auto size_in_byte = [=] {
        if(sec_cnt == 0) {
            return 256 * 512;
        }
        return sec_cnt * 512;
    }();

    insw(reg::data(hd->my_channel),buf,size_in_byte / 2);
}

// 将buf中sec_cnt扇区的数据写入磁盘
auto write_sector(disk* hd,void* buf,u8 sec_cnt) -> void
{
    auto size_in_byte = [=] {
        // 传入256的时候 u8的变量值直接就是0
        if(sec_cnt == 0) {
            return 256 * 512;
        }
        return sec_cnt * 512;
    }();

    outsw(reg::data(hd->my_channel),buf,size_in_byte / 2);
}

// 等待30秒 (是由一个人的发言，读写磁盘再慢，31秒也足够了)
// 返回等待后磁盘是否可以开始读写
auto busy_wait(disk* hd) -> bool
{
    auto channel = hd->my_channel;
    auto wait = i32(30s);
    while(wait > 0) {
        if(not (inb(reg::status(channel)) & STAT_BSY)) {
            return inb(reg::status(channel)) & STAT_DRQ;
        }
        sleep(10ms);
        wait -= 10;  // 每次减少10个tick（相当于10ms * 向上取整为100ms）
    }
    return false;
}

// 从硬盘读取sec_cnt个扇区到buf
auto ide_read(disk* hd,u32 lba,void* buf,u32 sec_cnt) -> void
{
    ASSERT(lba <= max_lba);
    ASSERT(sec_cnt > 0);

    auto lcg = lock_guard{ hd->my_channel->mtx };

    // 先选择硬盘
    select_disk(hd);

    for(auto k = 0u; k < sec_cnt; ) {
        // 单次操作的扇区数不能超过256
        auto op = std::min(sec_cnt - k,256u);
        // 写入待读入的扇区数和起始扇区号
        select_sector(hd,lba + k,op);
        // 执行的命令写入cmd寄存器
        cmd_out(hd->my_channel,CMD_READ_SECTOR);    // 准备开始读数据
        // 硬盘已经开始工作，现在选择阻塞自己
        hd->my_channel->disk_done.acquire();
        // 醒来后检测是否可读
        if(not busy_wait(hd)) {
            auto error = std::array<char,64>{};
            std::sprintf(error.data(),"{} read sector {} failed!!!\n",hd->name,lba);
            PANIC(error.data());
        }
        // 把数据从硬盘的缓冲区读出
        read_from_sector(hd,(void*)((u32)buf + k * 512),op);
        k += op;
    }

}

// 将buf中sec_cnt扇区数据写入硬盘
auto ide_write(disk* hd,u32 lba,void* buf,u32 sec_cnt) -> void
{
    ASSERT(lba <= max_lba);
    ASSERT(sec_cnt > 0);
    auto lcg = lock_guard{ hd->my_channel->mtx };
    select_disk(hd);
    for(auto k = 0u; k < sec_cnt; ) {
        auto op = std::min(sec_cnt - k,256u);
        select_sector(hd,lba + k,op);
        cmd_out(hd->my_channel,CMD_WRITE_SECTOR);
        if(not busy_wait(hd)) {
            auto error = std::array<char,64>{};
            std::sprintf(error.data(),"{} write sector {} failed!!!!!!\n",hd->name,lba);
            PANIC(error.data());
        }
        // 写入数据到硬盘
        write_sector(hd,(void*)((u32)buf + k * 512),op);
        // 在硬盘响应期间阻塞自己
        hd->my_channel->disk_done.acquire();
        k += op;
    }
}

// 获得硬盘参数信息
auto identify_disk(disk* hd) -> void
{
    auto id_info = std::array<char,512>{};
    select_disk(hd);
    cmd_out(hd->my_channel,CMD_IDENTIFY);
    hd->my_channel->disk_done.acquire();
    if(not busy_wait(hd)) {
        auto error = std::array<char,64>{};
        std::sprintf(error.data(),"{} identify failed!!!!!!\n",hd->name);
        PANIC(error.data());
    }
    read_from_sector(hd,id_info.data(),1);
    auto buf = std::array<char,64>{};
    auto constexpr sn_start = u8(10 * 2);
    auto constexpr sn_len = u8(20);
    auto constexpr md_start = u8(27 * 2);
    auto constexpr md_len = u8(40);
    std::pair_byte_swap(&id_info[sn_start],buf.data(),sn_len);
    console::println(buf.data());
    console::println("  disk {} info:\n     SN: {}",hd->name,buf.data());
    buf | std::fill[0]; // 等价于 std::fill(buf,0)
    std::pair_byte_swap(&id_info[md_start],buf.data(),md_len);
    console::println("      MODULE: {}",buf.data());
    auto sectors = *((u32*)&id_info[60 * 2]);
    console::println("      SECTORS: {}",sectors);
    console::println("      CAPACITY: {}MB",sectors * 512 / 1024 / 1024);
}