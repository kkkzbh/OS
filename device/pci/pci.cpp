module;

#include <assert.h>

export module pci;

import utility;

export struct pci_function
{
    u8 bus{};
    u8 slot{};
    u8 func{};
};

export auto constexpr PCI_INVALID_VENDOR = u16(0xffff);

export auto pci_read_u32(pci_function device,u8 offset) -> u32;
export auto pci_read_u16(pci_function device,u8 offset) -> u16;
export auto pci_read_u8(pci_function device,u8 offset) -> u8;
export auto pci_write_u32(pci_function device,u8 offset,u32 value) -> void;
export auto pci_write_u16(pci_function device,u8 offset,u16 value) -> void;
export auto pci_find_class_code(u8 class_code,u8 subclass,u8 prog_if,pci_function& out) -> bool;
export auto pci_bar_address(pci_function device,u8 bar_index) -> u32;
export auto pci_enable_bus_master_mmio(pci_function device) -> void;

namespace
{
    auto constexpr pci_config_address = u16(0xcf8);
    auto constexpr pci_config_data = u16(0xcfc);
    auto constexpr pci_command_memory_space = u16(0x0002);
    auto constexpr pci_command_bus_master = u16(0x0004);

    auto outl(u16 port,u32 value) -> void
    {
        asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
    }

    auto inl(u16 port) -> u32
    {
        auto value = u32{};
        asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
        return value;
    }

    auto config_address(pci_function device,u8 offset) -> u32
    {
        ASSERT((offset & 0x3) == 0);
        return 0x80000000u |
               (u32(device.bus) << 16) |
               (u32(device.slot) << 11) |
               (u32(device.func) << 8) |
               offset;
    }
}

auto pci_read_u32(pci_function device,u8 offset) -> u32
{
    auto const aligned = u8(offset & 0xfc);
    outl(pci_config_address,config_address(device,aligned));
    return inl(pci_config_data);
}

auto pci_read_u16(pci_function device,u8 offset) -> u16
{
    auto const shift = u32((offset & 0x2) * 8);
    return u16((pci_read_u32(device,u8(offset & 0xfc)) >> shift) & 0xffff);
}

auto pci_read_u8(pci_function device,u8 offset) -> u8
{
    auto const shift = u32((offset & 0x3) * 8);
    return u8((pci_read_u32(device,u8(offset & 0xfc)) >> shift) & 0xff);
}

auto pci_write_u32(pci_function device,u8 offset,u32 value) -> void
{
    auto const aligned = u8(offset & 0xfc);
    outl(pci_config_address,config_address(device,aligned));
    outl(pci_config_data,value);
}

auto pci_write_u16(pci_function device,u8 offset,u16 value) -> void
{
    auto const aligned = u8(offset & 0xfc);
    auto const shift = u32((offset & 0x2) * 8);
    auto reg = pci_read_u32(device,aligned);
    reg &= ~(0xffffu << shift);
    reg |= u32(value) << shift;
    pci_write_u32(device,aligned,reg);
}

auto pci_find_class_code(u8 class_code,u8 subclass,u8 prog_if,pci_function& out) -> bool
{
    for(auto bus = u16(0); bus != 256; ++bus) {
        for(auto slot = u8(0); slot != 32; ++slot) {
            auto const dev0 = pci_function{ u8(bus),slot,0 };
            if(pci_read_u16(dev0,0x00) == PCI_INVALID_VENDOR) {
                continue;
            }

            auto const header_type = pci_read_u8(dev0,0x0e);
            auto const func_count = (header_type & 0x80) ? u8(8) : u8(1);
            for(auto func = u8(0); func != func_count; ++func) {
                auto const device = pci_function{ u8(bus),slot,func };
                if(pci_read_u16(device,0x00) == PCI_INVALID_VENDOR) {
                    continue;
                }
                if(
                    pci_read_u8(device,0x0b) == class_code and
                    pci_read_u8(device,0x0a) == subclass and
                    pci_read_u8(device,0x09) == prog_if
                ) {
                    out = device;
                    return true;
                }
            }
        }
    }
    return false;
}

auto pci_bar_address(pci_function device,u8 bar_index) -> u32
{
    ASSERT(bar_index < 6);
    auto const bar = pci_read_u32(device,u8(0x10 + bar_index * 4));
    ASSERT((bar & 0x1) == 0);
    return bar & 0xfffffff0;
}

auto pci_enable_bus_master_mmio(pci_function device) -> void
{
    auto command = pci_read_u16(device,0x04);
    command |= pci_command_memory_space;
    command |= pci_command_bus_master;
    pci_write_u16(device,0x04,command);
}
