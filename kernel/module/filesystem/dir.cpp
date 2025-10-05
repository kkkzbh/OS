module;

#include <assert.h>
#include <string.h>

export module dir;

import utility;
import super_block;
import inode;
import array;
import filesystem.utility;
import ide;
import string;
import console;
import algorithm;
import dir.structure;
import filesystem;
import file;

// 根目录
export auto root = dir{};

export auto search_dir_entry(partition* part,dir* pdir,std::string_view<char const> name,dir_entry* dir_e) -> bool;

export auto dir_close(dir* dir) -> void;

export auto dir_open(partition* part,u32 inode_no) -> dir*;

auto open_root_dir(partition* part) -> void
{
    root.node = inode_open(part,part->sb->root_inode_no);
    root.pos = 0;
}

// 在分区part上打开i结点为inode_no的目录并返回目录指针
auto dir_open(partition* part,u32 inode_no) -> dir*
{
    return new dir {
        .node = inode_open(part,inode_no)
    };
}

// 在分区part内的pdir目录内寻找名为name的文件或目录
// 找到返回true，并将目录项存入dir_e，否则返回false
auto search_dir_entry(partition* part,dir* pdir,std::string_view<char const> name,dir_entry* dir_e) -> bool
{
    auto constexpr block_cnt = 140;     // 12个直接块 + 128个一级间接块
    // 12个直接块 + 128个间接块共560字节
    auto all_blocks = new std::array<u32,48 + 512>{};
    if(all_blocks == nullptr) {
        console::println("search_dir_entry: malloc for all_blocks failed!");
        return false;
    }
    auto& a = *all_blocks;
    auto& sectors = pdir->node->sectors;
    a | std::copy[sectors[0,sectors.size() - 1]];
    if(sectors.back() != 0) { // 若含有一级间接块表
        ide_read(part->my_disk,sectors.back(),a.data() + 12,1);
    }
    // 至此 all_blocs存储的是该文件或目录的所有扇区地址
    // 写目录项时保证目录项不夸扇区 读时只需要申请1个扇区的内存
    auto buf = new u8[SECTOR_SIZE];
    auto p_de = (dir_entry*)buf;
    // p_de为指向目录项的指针，值为buf起始地址
    auto dir_entry_size = part->sb->dir_entry_size;
    // 1扇区可容纳的目录项个数
    auto dir_entry_cnt = SECTOR_SIZE / dir_entry_size;
    // 在所有块中查找目录项
    for(auto i : std::iota[block_cnt]) {
        // 块地址为0表示块中无数据
        if(a[i] == 0) {
            continue;
        }
        ide_read(part->my_disk,a[i],buf,1);
        for(auto di : std::iota[dir_entry_cnt]) {
            if(std::string_view{ p_de->filename } == name) {
                *dir_e = *p_de;
                delete all_blocks;
                delete[] buf;
                return true;
            }
            ++p_de;
        }
        p_de = (dir_entry*)buf;
        std::subrange(buf,buf + SECTOR_SIZE) | std::fill[0];
    }
    delete all_blocks;
    delete[] buf;
    return false;
}

// 关闭目录
auto dir_close(dir* dir) -> void
{
    // 根目录打开后不应该关闭，否则需要再次open_root_dir
    // 根目录所在的内存是低端1MB之内，并非在堆中，free会出问题
    if(dir == &root) {
        return;
    }
    inode_close(dir->node);
    delete dir;
}

// 在内存中初始化目录项p_de
auto create_dir_entry(std::str auto filename,u32 inode_no,file_type type,dir_entry* p_de) -> void
{
    ASSERT(filename.size() <= MAX_FILES_NAME_LEN);
    p_de->filename | std::copy[filename];
    p_de->inode_no = inode_no;
    p_de->type = type;
}

// 将目录项p_de 写入父目录parent_dir中，io_buf由主调函数提供
auto sync_dir_entry(dir* parent_dir,dir_entry* p_de,void* buf) -> bool
{
    auto dir_inode = parent_dir->node;
    auto dir_size = dir_inode->size;
    auto dir_entry_size = cur_part->sb->dir_entry_size;
    // dir_size应该是dir_entry_size的整数倍
    ASSERT(dir_size % dir_entry_size == 0);
    // 每个扇区的最大目录项数
    auto dir_entrys_per_sec = 512 / dir_entry_size;
    // 12直接块 + 128间接块
    auto all_blocks = std::array<u32,12 + 128>{};
    auto& sectors = dir_inode->sectors;
    all_blocks | std::copy[sectors[0,sectors.size() - 1]];
    auto dir_e = (dir_entry*)buf;   // 用于在buf中遍历目录项
    for(auto i : std::iota[all_blocks.size()]) {
        if(all_blocks[i] == 0) {    // 在三种情况下分配块
            auto oblock_lba = block_bitmap_alloc(cur_part);
            if(not oblock_lba) {
                console::println("alloc block bitmap for sync_dir_entry failed!");
                return false;
            }
            auto block_lba = *oblock_lba;
            // 每分配一个块就同步一次block_bitmap
            auto block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
            ASSERT(block_bitmap_idx != -1);
            bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);
            if(i < 12) { // 直接块
                dir_inode->sectors[i] = all_blocks[i] = block_lba;
            } else if(i == 12) {    // 如果尚未分配一级间接块表
                // 分配的块作为一级块间接表地址
                dir_inode->sectors[12] = block_lba;
                // 再分配一个块作为第0个间接块
                oblock_lba = block_bitmap_alloc(cur_part);
                if(not oblock_lba) {    // 分配失败则还原状态
                    block_bitmap_idx = dir_inode->sectors[12] - cur_part->sb->data_start_lba;
                    cur_part->block.set(block_bitmap_idx,false);
                    dir_inode->sectors[12] = 0;
                    console::println("alloc block bitmao for sync_dir_entry failed!");
                    return false;
                }
                // 仍然每分配一个块就同步一次block_bitmap
                block_lba = *oblock_lba;
                block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
                ASSERT(block_bitmap_idx != -1);
                bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);
                all_blocks[12] = block_lba;
                // 把新分配的第0个间接块的地址写入一级间接块表
                ide_write(cur_part->my_disk,dir_inode->sectors[12],all_blocks.data() + 12,1);
            } else { // 间接块未分配
                all_blocks[i] = block_lba;
                // 把新分配的间接块地址写入一级间接块表
                ide_write(cur_part->my_disk,dir_inode->sectors[12],all_blocks.data() + 12,1);
            }
            // 再将新目录项p_de写入新分配的间接块
            memset(buf,0,512);
            memcpy(buf,p_de,dir_entry_size);
            ide_write(cur_part->my_disk,all_blocks[i],buf,1);
            dir_inode->size += dir_entry_size;
            return true;
        }
        // 如果第i块内存已存在，将其读入内存，然后再该块中查找空目录项
        ide_read(cur_part->my_disk,all_blocks[i],buf,1);
        for(auto dir_entry_i : std::iota[dir_entrys_per_sec]) {
            if((dir_e + dir_entry_i)->type == file_type::unknown) {
                // 无论是初始化还是删除文件后，都会把file_type置为unknown(0)
                memcpy(dir_e + dir_entry_i,p_de,dir_entry_size);
                ide_write(cur_part->my_disk,all_blocks[i],buf,1);
                dir_inode->size += dir_entry_size;
                return true;
            }
        }
    }
    console::println("directory is full!");
    return false;
}