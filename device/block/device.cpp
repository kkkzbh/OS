module;

#include <assert.h>

export module block.device;

import utility;
import array;
import string;

export struct block_device;

export using block_io_fn = auto (*)(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void;

export struct block_device
{
    char name[8]{};
    u32 sector_size{};
    u32 max_transfer{};
    block_io_fn read_blocks{};
    block_io_fn write_blocks{};
};

export auto constexpr MAX_BLOCK_DEVICES = size_t(4);

export auto block_device_cnt = u8{};
export std::array<block_device*,MAX_BLOCK_DEVICES> block_devices;

export auto reset_block_device_registry() -> void
{
    block_device_cnt = 0;
    for(auto& dev : block_devices) {
        dev = nullptr;
    }
}

export auto register_block_device(block_device* dev) -> void
{
    ASSERT(dev != nullptr);
    ASSERT(block_device_cnt < MAX_BLOCK_DEVICES);
    block_devices[block_device_cnt++] = dev;
}

export auto find_block_device(char const* name) -> block_device*
{
    if(name == nullptr) {
        return nullptr;
    }
    auto const wanted = std::string_view{ name };
    for(auto idx = u8(0); idx != block_device_cnt; ++idx) {
        auto* dev = block_devices[idx];
        if(dev != nullptr and std::string_view{ dev->name } == wanted) {
            return dev;
        }
    }
    return nullptr;
}

export auto block_read_blocks(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void
{
    ASSERT(dev != nullptr);
    ASSERT(dev->read_blocks != nullptr);
    ASSERT(block_cnt > 0);
    dev->read_blocks(dev,lba,buf,block_cnt);
}

export auto block_write_blocks(block_device* dev,u32 lba,void* buf,u32 block_cnt) -> void
{
    ASSERT(dev != nullptr);
    ASSERT(dev->write_blocks != nullptr);
    ASSERT(block_cnt > 0);
    dev->write_blocks(dev,lba,buf,block_cnt);
}
