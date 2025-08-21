

import utility;
import malloc;
import free;

auto operator new(size_t,void* place) noexcept -> void*
{
    return place;
}

auto operator new[](size_t,void* place) noexcept -> void*
{
    return place;
}

auto operator new(size_t sz) -> void*
{
    return malloc(sz);
}

auto operator delete(void* ptr) noexcept -> void
{
    free(ptr);
}

auto operator delete(void* ptr,size_t) noexcept -> void
{
    free(ptr);
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