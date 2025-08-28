

export module reference;

import utility;
import optional;

export template<typename T>
struct reference
{
    using value_type = T;

    auto static constexpr construct(T& r) -> T*
    {
        return &r;
    }

    auto constexpr construct(T&& r) = delete;

    constexpr reference() = default;

    template<typename U>
    explicit constexpr reference(U&& r) : it(reference::construct(std::forward<U>(r))) {}

    constexpr reference(reference const&) = default;

    auto constexpr operator=(reference const&) -> reference& = default;

    auto constexpr get() -> T&
    {
        return *it;
    }

    constexpr operator T&()
    {
        return get();
    }

    explicit constexpr operator bool() const
    {
        return get();
    }

    [[nodiscard]]
    auto friend constexpr
    operator==(reference x, reference y) -> bool
    { return x.get() == y.get(); }

    [[nodiscard]]
    auto friend constexpr
    operator==(reference x, T const& y) -> bool
    { return x.get() == y; }

    [[nodiscard]]
    auto friend constexpr
    operator==(reference x, reference<T const> y) -> bool
    { return x.get() == y.get(); }

    [[nodiscard]]
    auto friend constexpr
    operator<=>(reference x, reference y) -> std::strong_ordering
    { return x.get() <=> y.get(); }

    [[nodiscard]]
    auto friend constexpr
    operator<=>(reference x, T const& y) -> std::strong_ordering
    { return x.get() <=> y; }

    [[nodiscard]]
    auto friend constexpr
    operator<=>(reference x, reference<T const> y) -> std::strong_ordering
    { return x.get() <=> y.get(); }

    T* it = nullptr;
};

template<typename T>
struct reference<T&&>
{
    using value_type = T;

    constexpr reference() = default;

    explicit constexpr reference(T&& v) : it(std::move(v)) {}

    constexpr reference(reference&&) = default;

    auto constexpr operator=(reference&&) -> reference& = default;

    auto constexpr get() -> T&&
    {
        return move(*it);
    }

    constexpr operator T&&()
    {
        return get();
    }

    explicit constexpr operator bool() const
    {
        return it;
    }

    [[nodiscard]]
    auto friend constexpr
    operator==(reference x, reference y) -> bool
    { return x.get() == y.get(); }

    [[nodiscard]]
    auto friend constexpr
    operator==(reference x, T const& y) -> bool
    { return x.get() == y; }

    [[nodiscard]]
    auto friend constexpr
    operator==(reference x, reference<T const&&> y) -> bool
    { return x.get() == y.get(); }

    [[nodiscard]]
    auto friend constexpr
    operator<=>(reference x, reference y) -> std::strong_ordering
    { return x.get() <=> y.get(); }

    [[nodiscard]]
    auto friend constexpr
    operator<=>(reference x, T const& y) -> std::strong_ordering
    { return x.get() <=> y; }

    [[nodiscard]]
    auto friend constexpr
    operator<=>(reference x, reference<T const&&> y) -> std::strong_ordering
    { return x.get() <=> y.get(); }

    optional<T> it;
};

template<typename T,typename U>
auto constexpr operator==(reference<T> const& r1,reference<U> const& r2) -> bool
{
    return r1.get() == r2.get();
}

template<typename T>
reference(T&) -> reference<T>;

template<typename T>
reference(T&&) -> reference<T&&>;

export template<typename T>
auto ref(T& t) -> reference<T>
{
    return t;
}

export template<typename T>
auto ref(T&& t) -> reference<T&&>
{
    return std::move(t);
}

export template<typename T>
auto ref(reference<T> t) -> reference<T>
{
    return t;
}

template<typename T>
struct deref_s
{
    using value_type = T;
};

template<typename T>
struct deref_s<reference<T>>
{
    using value_type = T&;
};

template<typename T>
struct deref_s<reference<T&&>>
{
    using value_type = T&&;
};

template<typename T>
using deref = typename deref_s<T>::value_type;
