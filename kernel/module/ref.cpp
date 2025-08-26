

export module reference;

import utility;

export template<typename T>
struct reference
{
    using value_type = T;

    auto constexpr construct(T& r) -> T*
    {
        return &r;
    }

    auto constexpr construct(T&& r) = delete;

    template<typename U>
    constexpr reference(U&& r) : it(construct(r)) {}

    constexpr reference(reference const&) = default;

    auto constexpr operator=(reference const&) -> reference& = default;

    auto constexpr get() const -> T&
    {
        return *it;
    }

    constexpr operator T&() const
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

    T* it;
};

template<typename T>
reference(T&) -> reference<T>;

template<typename T>
auto ref(T& t) -> reference<T>
{
    return t;
}

template<typename T>
auto ref(T&& t) -> void = delete;

template<typename T>
auto ref(reference<T> t) -> reference<T>
{
    return t;
}