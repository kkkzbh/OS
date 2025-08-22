module;

#include <string.h>

export module string;

import utility;
import algorithm;

export namespace std
{
    template<typename T>
    concept CharT = std::same_as<T,char> or
                    std::same_as<T,char const> or
                    std::same_as<T,unsigned char> or
                    std::same_as<T,unsigned char const>;

    template<CharT Char>
    struct string_view
    {
        using value_type = Char;
        using iterator = Char*;
        using const_iterator = Char const*;

        constexpr string_view() = default;

        constexpr string_view(string_view const&) = default;

        constexpr string_view(Char* str,size_t count) : s(str),sz(count) {}

        template<random_range R>
        requires CharT<range_value_t<R>>
        constexpr string_view(R&& r) : s(&*begin(r)),sz(size(r)) {}

        string_view(nullptr_t) = delete("can not use nullptr constructor string_view");

        auto constexpr operator=(string_view const&) -> string_view& = default;

        auto constexpr begin(this auto&& self) -> decltype(auto)
        {
            return self.s;
        }

        auto constexpr end(this auto&& self) -> decltype(auto)
        {
            return self.s + self.sz;
        }

        auto constexpr operator[](this auto&& self,size_t idx) -> decltype(auto)
        {
            return self.s[idx];
        }

        auto constexpr front(this auto&& self) -> decltype(auto)
        {
            return self[0];
        }

        auto constexpr back(this auto&& self) -> decltype(auto)
        {
            return self[self.sz - 1];
        }

        auto constexpr data(this auto&& self) -> decltype(auto)
        {
            return self.s;
        }

        [[nodiscard]]
        auto constexpr size() const -> size_t
        {
            return sz;
        }

        [[nodiscard]]
        auto constexpr empty() const -> bool
        {
            return size() == 0;
        }

        auto constexpr swap(string_view other) -> void
        {
            swap(s,other.s);
            swap(sz,other.sz);
        }

        auto constexpr remove_prefix(size_t n) -> string_view
        {
            return { s + n,sz - n };
        }

        auto constexpr remove_suffix(size_t n) -> string_view
        {
            return { s,sz - n };
        }

        auto constexpr substr(size_t pos,size_t count) -> string_view
        {
            return { s + pos,count };
        }

        Char* s = nullptr;
        size_t sz = 0;
    };
}