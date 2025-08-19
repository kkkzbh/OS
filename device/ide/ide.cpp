

export module ide;

import utility;
import list;
import bitmap;
import mutex;
import semaphore;
import array;

// reg::alt_status寄存器的一些关键位
auto constexpr ALT_STAT_BSY = 0x80;     // 硬盘忙
auto constexpr ALT_STAT_DRAY = 0x40;    // 驱动器准备好
auto constexpr ALT_STAT_DRQ = 0x8;      // 数据传输准备好了

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

struct disk;
export struct ide_channel;

// 磁盘分区
struct partition
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
    auto data(ide_channel const& idec) -> u16
    {
        return idec.port_base + 0;
    }

    auto error(ide_channel const& idec) -> u16
    {
        return idec.port_base + 1;
    }

    auto sect_cnt(ide_channel const& idec) -> u16
    {
        return idec.port_base + 2;
    }

    auto lba_l(ide_channel const& idec) -> u16
    {
        return idec.port_base + 3;
    }

    auto lba_m(ide_channel const& idec) -> u16
    {
        return idec.port_base + 4;
    }

    auto lba_h(ide_channel const& idec) -> u16
    {
        return idec.port_base + 5;
    }

    auto dev(ide_channel const& idec) -> u16
    {
        return idec.port_base + 6;
    }

    auto status(ide_channel const& idec) -> u16
    {
        return idec.port_base + 7;
    }

    auto cmd(ide_channel const& idec) -> u16
    {
        return status(idec);
    }

    auto alt_status(ide_channel const& idec) -> u16
    {
        return idec.port_base + 0x206;
    }

    auto ctl(ide_channel const& idec) -> u16
    {
        return alt_status(idec);
    }

}