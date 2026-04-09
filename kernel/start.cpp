

#include <stdio.h>

auto init_all() -> void;
auto main() -> void;

import write_execution;
import ide.regression;
import thread;
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
        write_execution();  // 切记仅运行一次！
    }
    main();
    thread_exit(running_thread(),true);
}
