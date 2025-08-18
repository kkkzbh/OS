

export module getpid;

import utility;
import schedule;

export auto getpid() -> u32
{
    return running_thread()->pid;
}