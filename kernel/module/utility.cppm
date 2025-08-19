

export module utility;

export
{
    #include <stdint.h>
}

// 页表一页的大小 B
export auto constexpr PG_SIZE = 4096;

export enum struct thread_status
{
    running,
    ready,
    blocked,
    waiting,
    hanging,
    died
};

export template<typename T>
concept signed_integral =
    __is_same(T,i8) or
    __is_same(T,i16) or
    __is_same(T,i32);

export template<typename T>
concept unsigned_integral =
    __is_same(T,u8) or
    __is_same(T,u16) or
    __is_same(T,u32);

export template<typename T>
concept integral = signed_integral<T> or unsigned_integral<T>;

export using thread_function = void(void*);

export using pid_t = u16;

export namespace std
{
    template<typename T>
    struct remove_reference_s
    {
        using type = T;
    };

    template<typename T>
    struct remove_reference_s<T&>
    {
        using type = T;
    };

    template<typename T>
    struct remove_reference_s<T&&>
    {
        using type = T;
    };

    template<typename T>
    using remove_reference = typename remove_reference_s<T>::type;

    template<typename T>
    struct remove_const_s
    {
        using type = T;
    };

    template<typename T>
    struct  remove_const_s<T const>
    {
        using type = T;
    };

    template<typename T>
    using remove_const = typename remove_const_s<T>::type;

    template<typename T>
    using remove_cref = remove_reference<remove_const<T>>;

    template<typename T>
    struct is_lvalue_reference_s
    {
        auto static constexpr value = false;
    };

    template<typename T>
    struct is_lvalue_reference_s<T&>
    {
        auto static constexpr value = true;
    };

    template<typename T>
    struct is_lvalue_reference_s<T&&>
    {
        auto static constexpr value = false;
    };

    template<typename T>
    auto constexpr is_lvalue_reference = is_lvalue_reference_s<T>::value;

    template<typename T>
    struct is_rvalue_reference_s
    {
        auto static constexpr value = false;
    };

    template<typename T>
    struct is_rvalue_reference_s<T&>
    {
        auto static constexpr value = false;
    };

    template<typename T>
    struct is_rvalue_reference_s<T&&>
    {
        auto static constexpr value = true;
    };

    template<typename T>
    auto constexpr is_rvalue_reference = is_rvalue_reference_s<T>::value;

    template<typename T>
    using range_value_t = typename remove_cref<T>::value_type;

    template<typename T>
    auto constexpr move(T&& v) noexcept
    {
        return (remove_reference<T>&&)v;
    }

    template<typename T>
    auto constexpr forward(remove_reference<T>& v) noexcept -> T&&
    {
        return (T&&)v;
    }

    template<typename T>
    auto constexpr forward(remove_reference<T>&& v) noexcept -> T&& requires (not is_lvalue_reference<T>)
    {
        return (T&&)v;
    }

    template<typename T>
    auto constexpr div_ceil(T x,auto p)
    {
        return (x + p - 1) / p;
    }

    template<typename T,size_t N>
    auto constexpr begin(T (&a)[N]) -> auto*
    {
        return a;
    }

    template<typename T,size_t N>
    auto constexpr end(T (&a)[N]) -> auto*
    {
        return *(&a + 1);
    }

}