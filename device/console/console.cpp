

export module console;

import utility;
import thread;

namespace console
{

}

namespace console
{
    auto mtx = mutex{};

    auto write(char const* str) -> void
    {
        mtx.lock();
    }
}


