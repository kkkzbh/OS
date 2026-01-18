
export module wait_exit;

import task;
import algorithm;
import memory;
import alloc;
import list.node;
import filesystem.syscall;

// 释放用户进程资源
// 1. 页表中对应的物理页
// 2. 虚拟内存池占物理页框
// 3. 关闭打开的文件
export auto release_program_resource(task* thread) -> void
{
    u16 constexpr user_pde_nr = 768;
    u16 constexpr user_pte_nr = 1024;
    // 回收页表中用户空间的页框
    for(auto pde_i : std::iota[user_pde_nr]) {
        auto v_pde_ptr = thread->pgdir + pde_i;
        if(pgtable::contains(v_pde_ptr)) {    // p位为1，表示页目录存在，页目录项下可能有页表项
            auto first_pte_vaddr_in_pde = pgtable::pte_ptr(pde_i * 0x400000);   // 一个页表表示4MB
            for(auto pte_i : std::iota[user_pte_nr]) {
                auto v_pte_ptr = first_pte_vaddr_in_pde + pte_i;
                if(pgtable::contains(v_pte_ptr)) {
                    auto pg_phy_addr = *v_pde_ptr & 0xfffff000; // pte中记录的物理页框直接在对应内存池的位图清0
                    free_a_phy_page(pg_phy_addr);
                }
            }
            // 将pde中记录的物理页框直接在相应内存池的位图清0
            auto pg_phy_addr = *v_pde_ptr & 0xfffff000;
            free_a_phy_page(pg_phy_addr);
        }
    }
    // 回收用户虚拟地址池所占的物理内存
    auto const bitmap_pg_cnt = thread->userprog_vaddr.sz / PG_SIZE;
    auto const user_vaddr_pool_bitmap = thread->userprog_vaddr.bits;
    mfree_page(pool_flags::KERNEL,user_vaddr_pool_bitmap,bitmap_pg_cnt);
    // 关闭进程打开的文件
    for(auto fdi : std::iota[3,MAX_FILES_OPEN_PER_PROC]) {
        if(thread->fd_table[fdi] != -1) {
            close(fdi);
        }
    }
}

// 查找pnd的parent_pid是否是ppid
export auto find_child(list_node* pnd,i32 ppid) -> bool
{
    auto thread = find_task_by_all(pnd);
    return thread->parent_pid == ppid;
}

// 寻找状态为hanging的任务
export auto find_hanging_child(list_node* pnd,i32 ppid) -> bool
{
    auto thread = find_task_by_all(pnd);
    return thread->parent_pid == ppid and thread->stu == thread_status::hanging;
}

// 将一个子进程过继给init
export auto init_adopt_a_child(list_node* pnd,i32 pid) -> void
{
    auto thread = find_task_by_all(pnd);
    if(thread->parent_pid == pid) {
        thread->parent_pid = 1;
    }
}