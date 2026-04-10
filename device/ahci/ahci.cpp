module;

#include <assert.h>
#include <string.h>

export module ahci;

import utility;
import algorithm;
import array;
import bit;
import console;
import format;
import lock_guard;
import mutex;
import block.device;
import block.partition;
import pci;
import pci.mmio;
import alloc;
import memory;

export auto ahci_init() -> bool;

namespace
{
    auto constexpr sector_size = u32(512);
    auto constexpr max_transfer = u32(8);
    auto constexpr ahci_class_code = u8(0x01);
    auto constexpr ahci_subclass = u8(0x06);
    auto constexpr ahci_prog_if = u8(0x01);
    auto constexpr ahci_ghc_ae = u32(1u << 31);
    auto constexpr ahci_mmio_bytes = size_t(0x2000);
    auto constexpr ahci_port_count = size_t(32);
    auto constexpr sata_sig_ata = u32(0x00000101);
    auto constexpr hba_port_det_present = u32(0x3);
    auto constexpr hba_port_ipm_active = u32(0x1);
    auto constexpr hba_cmd_st = u32(1u << 0);
    auto constexpr hba_cmd_sud = u32(1u << 1);
    auto constexpr hba_cmd_pod = u32(1u << 2);
    auto constexpr hba_cmd_fre = u32(1u << 4);
    auto constexpr hba_cmd_fr = u32(1u << 14);
    auto constexpr hba_cmd_cr = u32(1u << 15);
    auto constexpr hba_port_tfd_bsy = u32(0x80);
    auto constexpr hba_port_tfd_drq = u32(0x08);
    auto constexpr hba_port_is_tfes = u32(1u << 30);
    auto constexpr fis_type_reg_h2d = u8(0x27);
    auto constexpr ata_cmd_identify = u8(0xec);
    auto constexpr ata_cmd_read_dma_ext = u8(0x25);
    auto constexpr ata_cmd_write_dma_ext = u8(0x35);
    auto constexpr command_timeout_spins = u32(5'000'000);
    auto constexpr device_ready_spins = u32(1'000'000);

    struct ahci_prdt_entry
    {
        u32 dba{};
        u32 dbau{};
        u32 reserved{};
        u32 dbc_i{};
    };

    struct ahci_command_table
    {
        std::array<u8,64> cfis{};
        std::array<u8,16> acmd{};
        std::array<u8,48> reserved{};
        std::array<ahci_prdt_entry,1> prdt_entry{};
    };

    struct ahci_command_header
    {
        u8 flags{};
        u8 flags2{};
        u16 prdtl{};
        u32 prdbc{};
        u32 ctba{};
        u32 ctbau{};
        std::array<u32,4> reserved{};
    };

    struct hba_port
    {
        u32 clb{};
        u32 clbu{};
        u32 fb{};
        u32 fbu{};
        u32 is{};
        u32 ie{};
        u32 cmd{};
        u32 reserved0{};
        u32 tfd{};
        u32 sig{};
        u32 ssts{};
        u32 sctl{};
        u32 serr{};
        u32 sact{};
        u32 ci{};
        u32 sntf{};
        u32 fbs{};
        std::array<u32,11> reserved1{};
        std::array<u32,4> vendor{};
    };

    struct hba_memory
    {
        u32 cap{};
        u32 ghc{};
        u32 is{};
        u32 pi{};
        u32 vs{};
        u32 ccc_ctl{};
        u32 ccc_pts{};
        u32 em_loc{};
        u32 em_ctl{};
        u32 cap2{};
        u32 bohc{};
        std::array<u8,0x74> reserved{};
        std::array<u8,0x60> vendor{};
        std::array<hba_port,32> ports{};
    };

    static_assert(sizeof(ahci_command_header) == 0x20);
    static_assert(sizeof(ahci_command_table) == 0x90);
    static_assert(sizeof(hba_port) == 0x80);
    static_assert(sizeof(hba_memory) == 0x1100);

    struct ahci_device
    {
        block_device base{};
        mutex mtx{};
        volatile hba_port* port{};
        u8 port_no{};
        void* command_list{};
        void* rfis{};
        void* command_table{};
        void* dma_buffer{};
        u32 command_list_phys{};
        u32 rfis_phys{};
        u32 command_table_phys{};
        u32 dma_buffer_phys{};
    };

    auto ahci_hba = static_cast<volatile hba_memory*>(nullptr);
    auto ahci_device_count = u8{};
    std::array<ahci_device,MAX_BLOCK_DEVICES> ahci_devices;

    auto ahci_read_blocks(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void;
    auto ahci_write_blocks(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void;

    auto as_ahci_device(block_device* dev) -> ahci_device*
    {
        ASSERT(dev != nullptr);
        return reinterpret_cast<ahci_device*>(dev);
    }

    auto phys_of(void* ptr) -> u32
    {
        ASSERT(ptr != nullptr);
        return pgtable::addr_v2p(reinterpret_cast<u32>(ptr));
    }

    auto port_has_sata_disk(volatile hba_port const* port) -> bool
    {
        auto const ssts = port->ssts;
        auto const det = ssts & 0x0f;
        auto const ipm = (ssts >> 8) & 0x0f;
        if(det != hba_port_det_present or ipm != hba_port_ipm_active) {
            return false;
        }
        return port->sig == sata_sig_ata;
    }

    auto stop_port(volatile hba_port* port) -> bool
    {
        port->cmd &= ~hba_cmd_st;
        port->cmd &= ~hba_cmd_fre;
        for(auto spin = 0u; spin != command_timeout_spins; ++spin) {
            if((port->cmd & (hba_cmd_cr | hba_cmd_fr)) == 0) {
                return true;
            }
        }
        return false;
    }

    auto start_port(volatile hba_port* port) -> void
    {
        port->cmd |= hba_cmd_sud | hba_cmd_pod;
        port->cmd |= hba_cmd_fre;
        port->cmd |= hba_cmd_st;
    }

    auto wait_device_ready(volatile hba_port* port) -> bool
    {
        for(auto spin = 0u; spin != device_ready_spins; ++spin) {
            if((port->tfd & (hba_port_tfd_bsy | hba_port_tfd_drq)) == 0) {
                return true;
            }
        }
        return false;
    }

    auto build_reg_h2d_fis(
        std::array<u8,64>& cfis,
        u8 command,
        u64 lba,
        u16 sector_count
    ) -> void
    {
        cfis | std::fill[u8(0)];
        cfis[0] = fis_type_reg_h2d;
        cfis[1] = u8(1u << 7);
        cfis[2] = command;
        cfis[4] = u8(lba >> 0);
        cfis[5] = u8(lba >> 8);
        cfis[6] = u8(lba >> 16);
        cfis[7] = u8(1u << 6);
        cfis[8] = u8(lba >> 24);
        cfis[9] = u8(lba >> 32);
        cfis[10] = u8(lba >> 40);
        cfis[12] = u8(sector_count & 0xff);
        cfis[13] = u8(sector_count >> 8);
    }

    auto issue_command(
        ahci_device* dev,
        u8 command,
        u64 lba,
        u16 sector_count,
        void* dma_buf,
        u32 byte_count,
        bool write
    ) -> bool
    {
        ASSERT(dev != nullptr);
        ASSERT(sector_count > 0);
        ASSERT(dma_buf != nullptr);
        auto* port = dev->port;
        ASSERT(port != nullptr);

        if(not wait_device_ready(port)) {
            return false;
        }

        auto* command_list = reinterpret_cast<ahci_command_header*>(dev->command_list);
        auto* header = &command_list[0];
        auto* table = reinterpret_cast<ahci_command_table*>(dev->command_table);
        memset(header,0,sizeof(*header));
        memset(table,0,PG_SIZE);

        header->flags = u8(5 | (write ? (1u << 6) : 0u));
        header->prdtl = 1;
        header->ctba = dev->command_table_phys;
        header->ctbau = 0;

        build_reg_h2d_fis(table->cfis,command,lba,sector_count);
        table->prdt_entry[0].dba = phys_of(dma_buf);
        table->prdt_entry[0].dbau = 0;
        table->prdt_entry[0].dbc_i = (byte_count - 1) | (1u << 31);

        port->is = 0xffffffff;
        port->ci = 1;
        for(auto spin = 0u; spin != command_timeout_spins; ++spin) {
            if((port->is & hba_port_is_tfes) != 0) {
                return false;
            }
            if((port->ci & 1) == 0) {
                return (port->is & hba_port_is_tfes) == 0;
            }
        }
        return false;
    }

    auto identify_device(ahci_device* dev) -> bool
    {
        auto* info = reinterpret_cast<char*>(dev->dma_buffer);
        memset(info,0,sector_size);
        if(not issue_command(dev,ata_cmd_identify,0,1,dev->dma_buffer,sector_size,false)) {
            return false;
        }

        auto model = std::array<char,64>{};
        auto const* words = reinterpret_cast<u16 const*>(info);
        std::pair_byte_swap(info + 27 * 2,model.data(),40);
        auto const sectors = u32(words[60]) | (u32(words[61]) << 16);
        console::println("  disk {} info:\n      MODEL: {}",dev->base.name,model.data());
        console::println("      SECTORS: {}",sectors);
        console::println("      CAPACITY: {}MB",sectors / 2048);
        return true;
    }

    auto init_device_runtime(ahci_device& dev,volatile hba_port* port,u8 port_no) -> bool
    {
        dev.base = {
            .sector_size = sector_size,
            .max_transfer = max_transfer,
        };
        dev.base.read_blocks = ahci_read_blocks;
        dev.base.write_blocks = ahci_write_blocks;
        dev.mtx.init();
        dev.port = port;
        dev.port_no = port_no;
        dev.command_list = get_kernel_pages(1);
        dev.rfis = get_kernel_pages(1);
        dev.command_table = get_kernel_pages(1);
        dev.dma_buffer = get_kernel_pages(1);
        if(
            dev.command_list == nullptr or
            dev.rfis == nullptr or
            dev.command_table == nullptr or
            dev.dma_buffer == nullptr
        ) {
            return false;
        }

        dev.command_list_phys = phys_of(dev.command_list);
        dev.rfis_phys = phys_of(dev.rfis);
        dev.command_table_phys = phys_of(dev.command_table);
        dev.dma_buffer_phys = phys_of(dev.dma_buffer);

        if(not stop_port(port)) {
            return false;
        }

        memset(dev.command_list,0,PG_SIZE);
        memset(dev.rfis,0,PG_SIZE);
        memset(dev.command_table,0,PG_SIZE);
        memset(dev.dma_buffer,0,PG_SIZE);

        port->clb = dev.command_list_phys;
        port->clbu = 0;
        port->fb = dev.rfis_phys;
        port->fbu = 0;
        port->ie = 0;
        port->serr = 0xffffffff;
        port->is = 0xffffffff;
        start_port(port);

        auto* headers = reinterpret_cast<ahci_command_header*>(dev.command_list);
        headers[0].ctba = dev.command_table_phys;
        headers[0].ctbau = 0;
        return true;
    }

    auto transfer_blocks(ahci_device* dev,u32 lba,void* buf,u32 block_cnt,bool write) -> void
    {
        ASSERT(dev != nullptr);
        ASSERT(buf != nullptr);
        auto* bytes = reinterpret_cast<u8*>(buf);
        auto lock = lock_guard{ dev->mtx };
        for(auto done = 0u; done < block_cnt;) {
            auto const chunk = std::min(block_cnt - done,max_transfer);
            auto const byte_count = chunk * sector_size;
            if(write) {
                memcpy(dev->dma_buffer,bytes + done * sector_size,byte_count);
            }
            auto const command = write ? ata_cmd_write_dma_ext : ata_cmd_read_dma_ext;
            if(not issue_command(dev,command,lba + done,chunk,dev->dma_buffer,byte_count,write)) {
                auto error = std::array<char,160>{};
                std::format_to(
                    error.data(),
                    "{} {} failed: port={} lba={} blocks={} is=0x{x} tfd=0x{x}\n",
                    dev->base.name,
                    write ? "write" : "read",
                    dev->port_no,
                    lba + done,
                    chunk,
                    dev->port->is,
                    dev->port->tfd
                );
                PANIC(error.data());
            }
            if(not write) {
                memcpy(bytes + done * sector_size,dev->dma_buffer,byte_count);
            }
            done += chunk;
        }
    }

    auto ahci_read_blocks(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void
    {
        ASSERT(block_cnt > 0);
        transfer_blocks(as_ahci_device(dev),lba,buf,block_cnt,false);
    }

    auto ahci_write_blocks(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void
    {
        ASSERT(block_cnt > 0);
        transfer_blocks(as_ahci_device(dev),lba,buf,block_cnt,true);
    }
}

auto ahci_init() -> bool
{
    console::println("ahci_init start");
    reset_block_device_registry();
    partition_runtime_init();
    ahci_device_count = 0;
    ahci_hba = nullptr;

    auto controller = pci_function{};
    if(not pci_find_class_code(ahci_class_code,ahci_subclass,ahci_prog_if,controller)) {
        console::println("ahci_init: no controller found");
        return false;
    }

    pci_enable_bus_master_mmio(controller);
    auto const abar = pci_bar_address(controller,5);
    if(abar == 0) {
        console::println("ahci_init: invalid BAR5");
        return false;
    }

    ahci_hba = reinterpret_cast<volatile hba_memory*>(mmio_map(abar,ahci_mmio_bytes));
    if(ahci_hba == nullptr) {
        console::println("ahci_init: mmio map failed");
        return false;
    }
    ahci_hba->ghc |= ahci_ghc_ae;
    ahci_hba->is = 0xffffffff;

    auto discovered = u8{};
    auto const port_bitmap = ahci_hba->pi;
    for(auto port_no = 0u; port_no != ahci_port_count and discovered < MAX_BLOCK_DEVICES; ++port_no) {
        if((port_bitmap & (1u << port_no)) == 0) {
            continue;
        }

        auto* port = &ahci_hba->ports[port_no];
        if(not port_has_sata_disk(port)) {
            continue;
        }

        auto& dev = ahci_devices[discovered];
        if(not init_device_runtime(dev,port,u8(port_no))) {
            console::println("ahci_init: port {} setup failed",port_no);
            continue;
        }
        std::format_to(dev.base.name,"sd{c}",'a' + discovered);
        if(not identify_device(&dev)) {
            console::println("ahci_init: port {} identify failed",port_no);
            continue;
        }

        register_block_device(&dev.base);
        if(discovered != 0) {
            scan_block_device_partitions(&dev.base);
        }
        ++discovered;
    }

    ahci_device_count = discovered;
    if(ahci_device_count == 0) {
        console::println("ahci_init: no SATA disks found");
        return false;
    }

    console::println("\n all partition info");
    partition_list | std::apply[partition_info];
    console::println("ahci_init done");
    return true;
}
