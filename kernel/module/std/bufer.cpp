module;

#include <string.h>

export module buffer;

import utility;
import string;
import algorithm;

namespace std
{
    export struct buffer
    {
        using value_type = char;
        using iterator = iter::random<value_type>;

        constexpr buffer() = default;

        template<random_range R>
        requires (same_as<range_value_t<R>,char> or same_as<range_value_t<R>,char unsigned>)
        constexpr explicit buffer(R&& r) : buf(&*std::begin(r)),sz(strlen(&*std::begin(r))) {}

        constexpr buffer(char* s,size_t len) : buf(s),sz(len) {}

        constexpr explicit buffer(char* s) : buffer(s,strlen(s)) {}

        template<typename T>
        constexpr buffer(string_view<T> str) requires (same_as<T,char> or same_as<T,char unsigned>)
        : buffer(str.data(),str.size()) {}

        constexpr buffer(buffer const&) = delete;

        auto constexpr operator=(buffer const&) -> buffer& = delete;

        constexpr explicit buffer(buffer&& b) noexcept : buf(b.buf),sz(b.sz)
        { b.buf = nullptr; }

        auto constexpr operator=(buffer&& b) noexcept -> buffer&
        {
            buf = b.buf;
            sz = b.sz;
            b.buf = nullptr;
            return *this;
        }

        auto constexpr operator[](size_t idx) const -> char&
        { return buf[idx]; }

        auto constexpr front(this buffer const& self) -> char&
        { return self[0]; }

        auto constexpr back(this buffer const& self) -> char&
        { return self[self.size() - 1]; }

        auto constexpr data() const -> char*
        { return buf;}

        auto constexpr begin() const -> iterator
        {
            return { buf };
        }

        auto constexpr end() const -> iterator
        {
            return { buf + sz };
        }

        auto constexpr size() const -> size_t
        { return sz; }

        auto constexpr empty() const -> bool
        { return sz == 0; }

        auto constexpr operator+=(char const* str) -> buffer&
        {
            strcat(buf + sz,str);
            return *this;
        }

        constexpr explicit operator string_view<char>()
        { return { buf,sz }; }

        template<random_range R> requires CharT<range_value_t<R>>
        auto constexpr operator+=(R&& r) -> buffer&
        {
            auto len = std::size(r);
            std::subrange{ buf + sz,buf + sz + len } | std::copy[r];
            sz += len;
            return *this;
        }

        char* buf = nullptr;
        size_t sz = 0;
    };
}
