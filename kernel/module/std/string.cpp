module;

#include <string.h>

export module string;

import utility;
import algorithm;

export namespace std
{
    struct string;

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

        constexpr string_view(string const& str);

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

    struct string
    {

        using value_type = char;
        using iterator = char*;
        using const_iterator = char const*;

        constexpr string() = default;

        constexpr string(string const& str) : buf(new char[str.size() + 1]),sz(str.size())
        {
            strcpy(buf,str.buf);
        }

        auto constexpr operator=(string const& str) -> string&
        {
            if(sz > str.size()) {
                strcpy(buf,str.buf);
                sz = str.size();
                return *this;
            }
            delete[] buf;
            sz = str.size();
            buf = new char[sz + 1];
            strcpy(buf,str.buf);
        }

        constexpr string(string&& str)
        {
            *this = move(str);
        }

        auto constexpr operator==(string&& str) -> string&
        {
            if(str.buf == buf) {
                return *this;
            }
            delete[] buf;
            buf = str.buf;
            str.buf = nullptr;
            sz = str.sz;
            return *this;
        }

        constexpr string(size_t count,char c) : buf(new char[count + 1]),sz(count)
        {
            string_view{ buf,sz } | fill[c];
            buf[sz] = '\0';
        }

        constexpr string(char const* str) : string(str,strlen(str))
        {}

        constexpr string(char const* str,size_t count) : buf(new char[count + 1]),sz(count)
        {
            string_view{ buf,sz } | copy[string_view{ str,count }];
        }

        string(nullptr_t) = delete;

        template<CharT Char>
        explicit constexpr string(string_view<Char> sv) : string(sv.data(),sv.size()) {}

        template<random_range R>
        requires CharT<range_value_t<R>>
        explicit constexpr string(R&& r) : string(&*r.begin(),r.size())
        {}

        auto constexpr data(this auto&& self) -> decltype(auto)
        {
            return self.buf;
        }

        [[nodiscard]]
        auto constexpr size() const -> size_t
        {
            return sz;
        }

        char* buf = nullptr;
        size_t sz = 0;
    };

    template<CharT Char>
    constexpr string_view<Char>::string_view(string const& str) : s(str.data()),sz(str.size()) {}
}