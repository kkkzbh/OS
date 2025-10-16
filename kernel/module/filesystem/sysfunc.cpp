module;

#include <assert.h>
#include <string.h>

export module filesystem.syscall;

import string;
import string.format;
import filesystem.utility;
import optional;
import console;
import dir;
import path;
import path.structure;
import file;
import file.manager;
import schedule;
import file.structure;
import array;
import algorithm;
import array.format;
import vector;
import filesystem;
import inode;
import scope;
import inode.structure;
import dir.structure;
import ide;

// 打开或创建文件成功后，返回文件描述符
export auto open(std::string_view<char const> pathname,u8 flags) -> optional<i32>
{
    if(pathname.back() == '/') { // 不支持打开目录
        console::println("can't open a directory {}",pathname);
        return {};
    }
    ASSERT(+flags <= 7);
    auto sr = path::search_record{};
    auto depth = path::depth(pathname.data());
    auto found = path::search(pathname.data(),&sr);
    if(sr.type == file_type::directory) {
        console::println("can't open a directory with open(), use opendir() to instead");
        dir_close(sr.parent_dir);
        return {};
    }
    auto cnt = path::depth(sr.path.data());
    // 先判断是否把pathname的各层目录都访问到了，即是否在某个中间目录失败
    if(cnt != depth) { // 说明某个中间目录不存在
        console::println("cannot access {}: Not a directory, subpath {} isn't exist",pathname,sr.path);
        dir_close(sr.parent_dir);
        return {};
    }
    // 如果在最后一个路径上没找到，并且不是要创建文件
    if(not found and not (+flags & +open_flags::create)) {
        console::println("in path {}, file {}  isn't exist",sr.path,strrchr(sr.path.data(),'/') + 1);
        dir_close(sr.parent_dir);
        return {};
    }
    if(found and +flags & +open_flags::create) { // 要创建的文件已经存在
        console::println("{} has already exist!",pathname);
        dir_close(sr.parent_dir);
        return {};
    }
    auto inode_no = *found;
    // 返回的fd是任务pcb->fd_table数组中的元素下标，而非全局file_table中的下标
    switch(+flags & +open_flags::create) {
        case +open_flags::create: {
            console::println("creating file");
            auto fd = file_create(sr.parent_dir,strrchr(pathname.data(),'/') + 1,+flags);
            dir_close(sr.parent_dir);
            return fd;
        } default: {    // 其余情况均为打开已存在文件
            auto fd = file_open(inode_no,flags);
            return fd;
        }
    }

    return {};

}

// 关闭文件
export auto close(i32 fdi) -> bool
{
    if(fdi <= 2) {
        return false;
    }
    auto global_fdi = fdi_local_to_global(fdi);
    auto ret = file_close(&file_table[global_fdi]);
    running_thread()->fd_table[fdi] = -1; // 使这个文件描述符位可用
    return ret;
}

// 将buf中连续count个字节写入文件描述符fd，返回写入的字节数
export auto write(i32 fd,void const* buf,u32 count) -> optional<i32>
{
    if(fd < 0) {
        console::println("sys_write: fd error!");
        return {};
    }
    auto buffer = std::subrange{ (char const*)(buf),(char const*)(buf) + count };
    if(fd == stdout) {
        auto a = std::array<char,1024>{};
        ASSERT(count <= 1024);
        a | std::copy[buffer];
        console::println("{}",a);
        return count;
    }
    auto global_fd = fdi_local_to_global(fd);
    auto& [pos,flag,node] = file_table[global_fd];
    if(+flag & +open_flags::write or +flag & +open_flags::rdwr) {
        return file_write(&file_table[global_fd],buf,count);
    }
    console::println("sys_write: not allowed to write file without flag rdwr or write");
    return {};
}

export auto read(i32 fd,void* buf,u32 count) -> optional<i32>
{
    if(fd < 0) {
        console::println("sys_read: fd error! fd should be greater_equal 0");
        return {};
    }
    ASSERT(buf);
    auto global_fd = fdi_local_to_global(fd);
    return file_read(&file_table[global_fd],buf,count);
}

// 重置用于文件读写操作的偏移指针，成功时返回新的偏移量
export auto lseek(i32 fd,i32 offset,whence flag) -> optional<i32>
{
    if(fd < 0) {
        console::println("sys_lseek: fd:(value {}) error",fd);
        return {};
    }
    auto global_fd = fdi_local_to_global(fd);
    auto pf = &file_table[global_fd];
    auto new_pos = [&] {
        using enum whence;
        switch(flag) {
            case set: {
                return offset;
            } case cur: {
                return i32(pf->pos) + offset;
            } case end: {
                // offset应该是负值
                return i32(pf->node->size) + offset;
            } default: {
                PANIC("whence error!");
            }
        }
        // unreachable
    }(); // NOLINT
    if(new_pos < 0 or new_pos >= i32(pf->node->size)) {
        console::println("the seek pos is error! value({}), which size is {}",new_pos,pf->node->size);
        return {};
    }
    pf->pos = new_pos;
    return pf->pos;
}

// 删除文件(非目录)，成功返回true
export auto unlink(std::string_view<char const> pathname) -> bool
{
    ASSERT(pathname.size() < MAX_PATH_LEN);
    // 先检查待删除的文件是否存在
    auto sr = path::search_record{};
    auto inode_no_opt = path::search(pathname.data(),&sr);
    if(not inode_no_opt) {
        console::println("file {} not found!",pathname);
        dir_close(sr.parent_dir);
        return false;
    }
    auto inode_no = *inode_no_opt;
    ASSERT(inode_no != 0);
    if(sr.type == file_type::directory) {
        console::println (
            "can't delete a directory with unlink(),"
            "use rmdir() to instead"
        );
        dir_close(sr.parent_dir);
        return false;
    }

    // 检查是否在已打开文件列表(文件表)中
    auto found = std::iota[MAX_FILE_OPEN] | std::first[([&](auto i) {
        auto& [pos,flag,node] = file_table[i];
        return node and inode_no == node->no;
    })];
    if(found) {
        dir_close(sr.parent_dir);
        console::println("file {} is in use, not allow to delete!\n",pathname);
        return false;
    }
    ASSERT(not found);
    auto iobuf = std::vector(SECTOR_SIZE + SECTOR_SIZE,char{});
    if(not iobuf) {
        dir_close(sr.parent_dir);
        console::println("sys_unlink: malloc for iobuf failed!");
        return {};
    }

    auto partent_dir = sr.parent_dir;
    delete_dir_entry(cur_part,partent_dir,inode_no,iobuf.data());
    inode_release(cur_part,inode_no);
    dir_close(sr.parent_dir);
    return true;
}

export auto mkdir(std::string_view<char const> pathname) -> bool
{
    auto active = true; // 用于回滚的scope使用
    auto iobuf = std::vector(SECTOR_SIZE * 2,char{});
    if(not iobuf) {
        console::println("sys_mkdir: sys_malloc for iobuf failed!");
        return false;
    }
    auto sr = path::search_record{};
    auto inode_no_opt = path::search(pathname.data(),&sr);
    auto close_parent_dir = scope_exit {
        [&] {
            dir_close(sr.parent_dir);
        },
        active
    };
    if(inode_no_opt) {      // 如果找到了同名目录或文件，失败返回
        console::println("sys_mkdir: file or directory {} exist!",pathname);
        return false;
    }
    // 如果没有找到，也要判断是否是某个中间目录不存在
    if(path::depth(pathname.data()) != path::depth(sr.path.data())) {
        // 说明没有访问到全部的路径，某个中间目录是不存在的
        console::println("sys_mkdir: cann't access {}: Not a directory, subpath {} isn't exist",pathname,sr.path);
        return false;
    }
    auto parent_dir = sr.parent_dir;
    // 目录名称后面可能有字符'/'，但是sr.path是不带的
    auto pname = std::string_view{ strrchr(sr.path.data(),'/') + 1 };
    auto inode_no = inode_bitmap_alloc(cur_part).value_or(-1);
    if(inode_no == -1) {
        console::println("sys_mkdir: allocate inode failed");
        return false;
    }
    auto free_bitmap_alloc = scope_exit {
        [&] {
            cur_part->inode.set(inode_no,false);
        },
        active
    };
    auto node = inode(inode_no);
    auto block_lba = block_bitmap_alloc(cur_part).value_or(-1);
    if(block_lba == -1) {
        console::println("sys_mkdir: block_bitmap_alloc for create directory failed!");
        return {};
    }
    node.sectors.front() = block_lba;
    // 每分配一个块就将位图同步到硬盘
    auto block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
    ASSERT(block_bitmap_idx != 0);
    bitmap_sync(cur_part,block_bitmap_idx,bitmap_type::block);

    // 将当前目录项"." ".." 写入目录
    iobuf | std::fill[decltype(iobuf)::value_type{}];
    auto p_de = (dir_entry*)(iobuf.data());
    p_de->filename | std::copy["."];
    p_de->inode_no = inode_no;
    p_de->type = file_type::directory;
    ++p_de;
    p_de->filename | std::copy[".."];
    p_de->inode_no = parent_dir->node->no;
    p_de->type = file_type::directory;
    ide_write(cur_part->my_disk,node.sectors.front(),iobuf.data(),1);
    node.size = 2 * cur_part->sb->dir_entry_size;

    // 在父目录中添加自己的目录项
    auto entry = dir_entry{};
    create_dir_entry(pathname,inode_no,file_type::directory,&entry);
    iobuf | std::fill[0];
    // 将block_bitmap通过bitmap_sync同步到硬盘
    if(not sync_dir_entry(parent_dir,&entry,iobuf.data())) {
        console::println("sys_mkdir: sync_dir_entry to disk failed!");
        return false;   // 如果这里返回可能会引起块泄露！！*******************************
    }

    // 父目录的inode同步到硬盘
    iobuf | std::fill[0];
    inode_sync(cur_part,parent_dir->node,iobuf.data());
    // 将新创建目录的inode同步到硬盘
    iobuf | std::fill[0];
    inode_sync(cur_part,&node,iobuf.data());
    // 将inode位图同步到硬盘
    bitmap_sync(cur_part,inode_no,bitmap_type::inode);
    // 关闭所创建目录的父目录
    dir_close(sr.parent_dir);
    // 取消回滚
    active = false;
    return true;
}

// 打开目录，返回 dir*，失败返回空指针
export auto opendir(std::string_view<char const> pathname) -> dir*
{
    ASSERT(pathname.size() < MAX_PATH_LEN);
    // 如果是根目录直接返回根目录
    if(pathname == "/"sv or pathname == "/."sv) {
        return &root;
    }
    // 确保该目录是存在的
    auto sr = path::search_record{};
    auto inode_no = path::search(pathname.data(),&sr).value_or(-1);
    auto active = true;
    auto close_parent_dir = scope_exit {
        [&] {
            dir_close(sr.parent_dir);
        },
        active
    };
    if(inode_no == -1) {
        console::println("open {} filed, the subpath {} is not exist!",pathname,sr.path);
        return nullptr;
    }
    if(sr.type == file_type::regular) {
        console::println("{} is a regular file!",pathname);
        return nullptr;
    }
    return dir_open(cur_part,inode_no);
}

//关闭目录dir，成功返回true
export auto closedir(dir* dir) -> bool
{
    if(dir == nullptr) {
        return false;
    }
    dir_close(dir);
    return true;
}

// 读取dir的一个目录项，成功返回目录项地址，失败或到目录尾返回nullptr
export auto readdir(dir* dir) -> dir_entry*
{
    ASSERT(dir != nullptr);
    return dir_read(dir);
}

// 把目录dir的指针pos置0
export auto rewinddir(dir* dir) -> void
{
    dir->pos = 0;
}