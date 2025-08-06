

export module sync:lock_guard;

import :mutex;

export template<typename Mutex>
struct lock_guard
{
    lock_guard(lock_guard const&) = delete;
    auto operator=(lock_guard const&) -> lock_guard& = delete;

    explicit constexpr lock_guard(Mutex& mtx) : mutex(mtx)
    {
        mutex.lock();
    }

    ~lock_guard()
    {
        mutex.unlock();
    }

    Mutex& mutex;
};