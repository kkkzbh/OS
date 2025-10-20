module;

#include <string.h>

export module string;

import utility;
import algorithm;
import optional;


export namespace std
{

    template<typename T>
    concept CharT = std::same_as<T,char> or
                std::same_as<T,char const> or
                std::same_as<T,unsigned char> or
                std::same_as<T,unsigned char const> or
                std::same_as<T,char signed> or
                std::same_as<T,char signed const>;

    template<CharT Char>
    struct string_view;

    auto constexpr operator==(string_view<char const> x,string_view<char const> y) -> bool;
    auto constexpr operator==(string_view<char const> x,decltype(nullptr))-> bool;

    template<CharT Char>
    struct string_view
    {
        using value_type = Char;
        using iterator = std::iter::random<Char>;

        constexpr string_view() = default;

        constexpr string_view(string_view const&) = default;

        constexpr string_view(Char* str) : string_view(str,strlen(str)) {}

        constexpr string_view(Char* str,size_t count) : s(str),sz(count) {}

        template<random_range R>
        requires CharT<range_value_t<R>>
        constexpr string_view(R&& r) : s(&*std::begin(r)),sz(strlen(&*std::begin(r))) {}

        string_view(nullptr_t) = delete("can not use nullptr constructor string_view");

        constexpr operator string_view<char const>() const
        { return { s,sz }; }

        auto constexpr operator=(string_view const&) -> string_view& = default;

        auto constexpr begin(this auto&& self) -> decltype(auto)
        {
            return iterator{ self.s };
        }

        auto constexpr end(this auto&& self) -> decltype(auto)
        {
            return iterator{ self.s + self.sz };
        }

        auto constexpr operator[](this auto&& self,size_t idx) -> decltype(auto)
        {
            return self.s[idx];
        }

        auto constexpr operator[](this auto&& self,size_t x,size_t y) -> decltype(auto)
        {
            return self.substr(x,y - x);
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

        auto constexpr operator*() -> decltype(auto)
        {
            return front();
        }

        auto constexpr operator++() -> string_view&
        {
            ++s;
            return *this;
        }

        auto constexpr operator--() -> string_view&
        {
            --s;
            return *this;
        }

        auto constexpr starts_with(string_view<char const> sv) const noexcept -> bool
        {
            return size() >= sv.size() and string_view<char const>{ s,sv.size() } == sv;
        }

        auto constexpr ends_with(string_view<char const> sv) const noexcept -> bool
        {
            return size() >= sv.size() and string_view<char const>{ s + (size() - sv.size()),sv.size() } == sv;
        }

        auto constexpr find(string_view<char const> sv,size_t pos = 0) const noexcept -> optional<size_t>
        {
            auto i = pos;
            auto bound = sz - sv.size() + 1;
            auto my = *this->remove_prefix(i);
            my.sz = sv.size();
            for(; i < bound; ++i,++my.s) {
                if(my == sv) {
                    return i;
                }
            }
            return nullopt;
        }

        auto constexpr contains(string_view<char const> sv) const noexcept -> bool
        {
            return find(sv);
        }

        Char* s = nullptr;
        size_t sz = 0;
    };

    auto constexpr operator<=>(string_view<char const> x,string_view<char const> y) -> strong_ordering
    {
        using enum strong_ordering;
        auto bound = std::min(x.size(),y.size());
        return [=] {
            for(auto i = 0; i != bound; ++i) {
                if(x[i] == y[i]) {
                    continue;
                }
                if(x[i] < y[i]) {
                    return less;
                }
                return greater;
            }
            if(x.size() < y.size()) {
                return less;
            }
            if(x.size() > y.size()) {
                return greater;
            }
            return equal;
        }();
    }

    auto constexpr operator==(string_view<char const> x,string_view<char const> y) -> bool
    {
        if(x.size() != y.size()) {
            return false;
        }
        return (x <=> y) == 0;
    }

    auto constexpr operator==(string_view<char const> x,decltype(nullptr))-> bool
    {
        return x.s == nullptr;
    }

    template<typename T>
    concept str = requires(T t) {
        typename T::value_type;
        requires std::same_as<T,std::string_view<typename T::value_type>>;
    };

    template<random_range R>
    requires CharT<range_value_t<R>>
    string_view(R&& r) -> string_view<range_value_t<R>>;

}

export auto constexpr operator""sv(char const* str,size_t sz) -> std::string_view<char const>   // NOLINT
{
    return { str,sz };
}

