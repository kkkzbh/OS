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
import file.structure;
import file.manager;
import vector;

using namespace fs;

// 根目录
export auto root = dir{};

export auto search_dir_entry(partition* part,dir* pdir,std::string_view<char const> name,dir_entry* dir_e) -> bool;
export auto dir_close(dir* dir) -> void;
export auto dir_open(partition* part,u32 inode_no) -> dir*;
export auto create_dir_entry(std::str auto filename,u32 inode_no,file_type type,dir_entry* p_de) -> void;
export auto sync_dir_entry(dir* parent_dir,dir_entry* p_de,void* buf) -> bool;
export auto open_root_dir(partition* part) -> void;
export auto delete_dir_entry(partition* part,dir* pdir,u32 inode_no,void* iobuf) -> bool;
export auto dir_read(dir* dir) -> dir_entry*;
export auto dir_is_empty(dir* dir) -> bool;
export auto dir_remove(dir* parent_dir,dir* child_dir) -> bool;
export auto get_parent_dir_inode_no(u32 child_inode_no,void* iobuf) -> u32;
export auto get_child_dir_name(u32 p_inode_no,u32 c_inode_no,char* path,void* iobuf) -> bool;

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

// 把分区part目录pdir中编号为inode_no的目录项删除
auto delete_dir_entry(partition* part,dir* pdir,u32 inode_no,void* iobuf) -> bool
{
    auto dir_inode = pdir->node;
    auto all_blocks = std::array<u32,128 + 12>{};
    // 收集目录块全部地址
    for(auto block_idx : std::iota[12]) {
        all_blocks[block_idx] = dir_inode->sectors[block_idx];
    }
    if(dir_inode->sectors.back() != 0) {
        ide_read(part->my_disk,dir_inode->sectors.back(),all_blocks.data() + 12,1);
    }
    // 目录项在存储时保证不跨扇区
    auto dir_entry_size = part->sb->dir_entry_size;
    // 每扇区最大的目录项数目
    auto dir_entrys_per_sec = SECTOR_SIZE / dir_entry_size;
    auto dir_e = (dir_entry*)(iobuf);
    auto is_dir_first_block = false;    // 目录的第1个块

    for(auto block_idx : std::iota[140]) {
        if(all_blocks[block_idx] == 0) {
            continue;
        }
        memset(iobuf,0,SECTOR_SIZE);
        // 读取扇区，获得目录项
        ide_read(part->my_disk,all_blocks[block_idx],iobuf,1);
        is_dir_first_block = false;
        // 遍历所有的目录项
        auto dir_entry_cnt = 0;
        auto dir_entry_found = (dir_entry*)(nullptr);
        for(auto dir_entry_idx : std::iota[dir_entrys_per_sec]) {
            auto& entry = dir_e[dir_entry_idx];
            if(entry.type == file_type::unknown) {
                continue;
            }
            auto filename = std::string_view{ entry.filename };
            if(filename == "."sv) {
                is_dir_first_block = true;
            } else if(filename != ".."sv) {
                // 统计此扇区内的目录项个数，用来判断删除目录项后是否回收该扇区
                ++dir_entry_cnt;
                if(entry.inode_no == inode_no) {
                    // 确保目录中只有一个编号为inode_n的inode
                    ASSERT(dir_entry_found == nullptr);
                    // 找到该i结点，就记录
                    dir_entry_found = dir_e + dir_entry_idx;
                }
            }
        }
        // 如果这个扇区没找到，就继续去下个扇区找
        if(dir_entry_found == nullptr) {
            continue;
        }
        // 在此扇区中找到目录项后，清除该目录项并判断是回收扇区
        ASSERT(dir_entry_cnt >= 1);
        // 除了目录第1个扇区外，若该扇区上只有该目录项自己，则将整个扇区回收
        if(dir_entry_cnt == 1 and not is_dir_first_block) {
            // 在块位图回收该块
            auto block_bitmap_idx = all_blocks[block_idx] - part->sb->data_start_lba;
            part->block.set(block_bitmap_idx,false);
            bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);

            // 将块地址从数组sectors或索引表中去掉
            if(block_idx < 12) {
                dir_inode->sectors[block_idx] = 0;
            } else {    // 在一级间接索引表中擦除该间接块地址
                // 先判断一级间接索引表中间块的数量，如果仅有这1个间接块，连同间接索引表所在的块一同回收
                auto indirect_blocks = 0;
                for(auto indirect_block_idx : std::iota[12,140]) {
                    indirect_blocks += all_blocks[indirect_block_idx] != 0;
                }
                ASSERT(indirect_blocks >= 1);   // 包括当前间接块
                if(indirect_blocks > 1) {
                    // 间接索引表还包括其他间接块，仅在索引表中擦除这个间接块地址
                    all_blocks[block_idx] = 0;
                    ide_write(part->my_disk,dir_inode->sectors.back(),all_blocks.data() + 12,1);
                } else {    // 间接索引表就当前这1个间接块
                    // 直接把间接索引表所在的块回收，然后擦除简介索引表块地址
                    block_bitmap_idx = dir_inode->sectors.back() - part->sb->data_start_lba;
                    part->block.set(block_idx,false);
                    bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);
                    // 将间接索引表地址清0
                    dir_inode->sectors.back() = 0;
                }
            }
        } else {    // 仅将该目录项清空
            *dir_entry_found = {};
            ide_write(part->my_disk,all_blocks[block_idx],iobuf,1);
        }

        // 更新i结点信息并同步到硬盘
        ASSERT(dir_inode->size >= dir_entry_size);
        dir_inode->size -= dir_entry_size;
        memset(iobuf,0,SECTOR_SIZE * 2);
        inode_sync(part,dir_inode,iobuf);
        return true;
    }

    // 所有块中都没找到返回false，一般为search_file出错
    return false;
}

// 读取目录，成功返回一个目录项，失败返回nullptr
auto dir_read(dir* dir) -> dir_entry*
{
    auto dir_e = (dir_entry*)(dir->buf.data());
    auto dir_inode = dir->node;
    auto all_blocks = std::array<u32,128 + 12>{};
    auto block_cnt = 12;
    for(auto block_idx : std::iota[12]) {
        all_blocks[block_idx] = dir_inode->sectors[block_idx];
    }
    if(dir_inode->sectors.back() != 0) { // 若含有一级间接块表
        ide_read(cur_part->my_disk,dir_inode->sectors.back(),all_blocks.data() + 12,1);
        block_cnt = 140;
    }
    auto dir_entry_size = cur_part->sb->dir_entry_size;
    auto dir_entrys_per_sec = SECTOR_SIZE / dir_entry_size; // 1个扇区可容纳的目录项个数

    auto block_idx = 0;
    auto cur_dir_entry_pos = 0;     // 记录当前目录的偏移，要与目录实际偏移对应
    while(dir->pos < dir_inode->size) {
        if(all_blocks[block_idx] == 0) {    // 如果此地址块是空块，直接下一块
            ++block_idx;
            continue;
        }
        dir->buf | std::fill[0];
        ide_read(cur_part->my_disk,all_blocks[block_idx],dir_e,1);
        for(auto dir_entry_idx : std::iota[dir_entrys_per_sec]) {   // 遍历扇区内所有目录项
            if(dir_e[dir_entry_idx].type == file_type::unknown) {   // 如果是未知目录项
                continue;
            }
            if(cur_dir_entry_pos < dir->pos) {  // 如果当前的偏移还不是目录偏移，这里判断防止返回已经返回过的目录项
                cur_dir_entry_pos += dir_entry_size;
                continue;
            }
            ASSERT(cur_dir_entry_pos == dir->pos);
            dir->pos += dir_entry_size;     // 更新为新位置，下一个要返回目录项的地址
            return dir_e + dir_entry_idx;   // 返回这个目录项
        }
    }
    return nullptr; // 没找到，返回空指针表示没有目录项了
}

// 判断目录是否为空
auto dir_is_empty(dir* dir) -> bool
{
    auto dir_inode = dir->node;
    // 如果目录下只有.和..两个目录项，则为空的
    return dir_inode->size == cur_part->sb->dir_entry_size * 2;
}

// 在父目录里删除子目录
auto dir_remove(dir* parent_dir,dir* child_dir) -> bool
{
    auto child_dir_inode = child_dir->node;
    // 空目录只在sectors.front()中有扇区，其他都应为空
    for(auto block_idx : std::iota[1,13]) {
        ASSERT(child_dir_inode->sectors[block_idx] == 0);
    }
    auto iobuf = std::vector(SECTOR_SIZE * 2,char{});
    if(not iobuf) {
        console::println("dir_remove: malloc for iobuf failed!");
        return false;
    }
    // 在父目录parent_dir中删除子目录child_dir对应的目录项
    delete_dir_entry(cur_part,parent_dir,child_dir_inode->no,iobuf.data());
    // 回收inode中sector所占用的扇区
    inode_release(cur_part,child_dir_inode->no);
    return true;
}

// 获取父目录的inode编号
auto get_parent_dir_inode_no(u32 child_inode_no,void* iobuf) -> u32
{
    auto child_dir_inode = inode_open(cur_part,child_inode_no);
    // 目录中的目录项..中包括父目录的inode编号，..位于目录的第0块
    auto block_lba = child_dir_inode->sectors.front();
    ASSERT(block_lba >= cur_part->sb->data_start_lba);
    inode_close(child_dir_inode);
    ide_read(cur_part->my_disk,block_lba,iobuf,1);
    auto dir_e = (dir_entry*)(iobuf);
    // 第0个目录项是"."，第一个是".."
    ASSERT(dir_e[1].inode_no < 4096 and dir_e[1].type == file_type::directory);
    return dir_e[1].inode_no;
}

// 在inode编号为p_inode_no的目录中查找inode编号为c_inode_no的子目录的名字，将名字写入缓冲区path
auto get_child_dir_name(u32 p_inode_no,u32 c_inode_no,char* path,void* iobuf) -> bool
{
    auto parent_dir_inode = inode_open(cur_part,p_inode_no);
    // 填充all_blocks，将该目录所占的扇区地址全部写入all_blocks
    auto block_cnt = 12;
    auto all_blocks = std::array<u32,128 + 12>{};
    for(auto block_idx : std::iota[12]) {
        all_blocks[block_idx] = parent_dir_inode->sectors[block_idx];
    }
    if(parent_dir_inode->sectors.back() != 0) { // 如果包含了一级间接表
        ide_read(cur_part->my_disk,parent_dir_inode->sectors.back(),all_blocks.data() + 12,1);
        block_cnt = 140;
    }
    inode_close(parent_dir_inode);
    auto dir_e = (dir_entry*)(iobuf);
    auto dir_entry_size = cur_part->sb->dir_entry_size;
    auto dir_entrys_per_sec = 512 / dir_entry_size;
    for(auto block_idx : std::iota[block_cnt]) {    // 遍历所有块
        if(not all_blocks[block_idx]) { // 如果该块为空
            continue;
        }
        ide_read(cur_part->my_disk,all_blocks[block_idx],iobuf,1);  // 读入该块
        for(auto dir_e_idx : std::iota[dir_entrys_per_sec]) {   // 遍历每个目录项
            if(dir_e[dir_e_idx].inode_no != c_inode_no) {   // 查找目录本身
                continue;
            }
            strcat(path,"/");
            strcat(path,dir_e[dir_e_idx].filename.data());
            return true;
        }
    }
    return false;
}