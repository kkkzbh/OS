module;

#include <stdio.h>

export module optional;

import utility;

struct nullopt_t
{
    enum class _Construct
    {
        _Token
    };

    explicit constexpr nullopt_t(_Construct) {}
};

export constexpr inline auto nullopt = nullopt_t{ nullopt_t::_Construct::_Token };

export template<typename T>
struct optional
{

    constexpr optional() = default;

    constexpr optional(nullopt_t) {}

    constexpr optional(optional const& other) = default;

    constexpr optional(T const& v) : has(true), value(v) {}

    constexpr optional(T&& v) : has(true), value(static_cast<T&&>(v)) {}

    auto constexpr operator*(this auto&& self) -> decltype(auto)
    { return self.value; }

    explicit constexpr operator bool() const
    { return has; }

    auto constexpr value_or(this auto&& self,T const& default_value) -> decltype(auto)
    {
        if(self) {
            return *self;
        }
        return default_value;
    }

private:
    bool has = false;
    T value;
};