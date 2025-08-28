


import sys;
import utility;

auto operator new(size_t sz) -> void*
{
    return std::malloc(sz);
}

auto operator delete(void* ptr) noexcept -> void
{
    std::free(ptr);
}

auto operator delete(void* ptr,size_t) noexcept -> void
{
    std::free(ptr);
}

auto operator new[](size_t sz) -> void*
{
    return operator new(sz);
}

auto operator delete[](void* ptr) noexcept -> void
{
    operator delete(ptr);
}

auto operator delete[](void* ptr,size_t) noexcept -> void
{
    operator delete(ptr);
}