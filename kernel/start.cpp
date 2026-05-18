

#include <stdio.h>

auto init_all() -> void;
auto main() -> void;

import write_execution;
import storage.regression;
import thread;
import thread.init;
import schedule;

extern "C" auto start() -> void
{
    write_boot_marker_slot(3, "BOOT:K1");
    init_all();
    clear();
    write_boot_marker_slot(3, "BOOT:K1");
    if(auto const case_name = disk_regression_mode_case()) {
        run_disk_regression(case_name);
    } else if(not disk_regression_mode_active()) {
        write_execution();  // 在 shell/init 进程启动前导入用户程序，避免和 exec 并发访问同一文件
        start_init_process();
    }
    main();
    thread_exit(running_thread(),true);
}
