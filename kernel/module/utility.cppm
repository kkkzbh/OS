

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

export using thread_function = void(void*);

export using pid_t = u16;

export namespace std
{
    // NOLINTBEGIN

    template<typename T>
    concept signed_integral =
        __is_same(T,i8) or
        __is_same(T,i16) or
        __is_same(T,i32);

    template<typename T>
    concept unsigned_integral =
        __is_same(T,u8) or
        __is_same(T,u16) or
        __is_same(T,u32);

    template<typename T>
    concept integral = signed_integral<T> or unsigned_integral<T>;

    template<typename T,typename U>
    auto constexpr inline is_same = __is_same(T,U);

    template<typename T,typename U>
    concept same_as = (is_same<T,U> and is_same<U,T>);

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
    concept lvalue_reference = is_lvalue_reference<T>;

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
    concept rvalue_reference = is_lvalue_reference<T>;

    template<typename T>
    struct is_value_t_s
    {
        auto static constexpr value = true;
    };

    template<typename T>
    struct is_value_t_s<T&>
    {
        auto static constexpr value = false;
    };

    template<typename T>
    struct is_value_t_s<T&&>
    {
        auto static constexpr value = false;
    };

    template<typename T>
    auto constexpr is_value_t = is_value_t_s<T>::value;

    template<typename T>
    concept value_t = is_value_t<T>;

    template<typename T>
    struct remove_lvalue_reference_s
    {
        using value_type = T;
    };

    template<typename T>
    struct remove_lvalue_reference_s<T&>
    {
        using value_type = T;
    };

    template<typename T>
    struct remove_lvalue_reference_s<T&&>
    {
        using value_type = T&&;
    };

    template<typename T>
    using remove_lvalue_reference = remove_lvalue_reference_s<T>::value_type;

    template<typename T>
    using range_value_t = typename remove_cref<T>::value_type;

    template<typename It>
    struct iter_traits_s {};

    template<typename It>
    struct iter_traits_s<It*>
    {
        using value_type = It;
    };

    template<typename It>
    requires requires(remove_cref<It> it) {
        typename remove_cref<It>::value_type;
    }
    struct iter_traits_s<It>
    {
        using value_type = typename remove_cref<It>::value_type;
    };

    template<typename It>
    using iter_value_t = typename iter_traits_s<It>::value_type;

    template<typename T>
    using range_iter_t = typename remove_cref<T>::iterator;

    template<typename It>
    concept input_iterator = requires(remove_cref<It> it) {
        ++it;
        { *it };
    };

    template<typename It>
    concept output_iterator = requires(remove_cref<It> it,iter_value_t<It> value) {
        ++it;
        *it = value;
    };

    template<typename It>
    concept forward_iterator = input_iterator<It> and output_iterator<It>;

    template<typename It>
    concept random_iterator = forward_iterator<It> and requires(remove_cref<It> it,int n) {
        { it[0] };
        it += n;
    };

    template<typename R>
    concept range = requires(R r) {
        begin(r);
        end(r);
    };

    template<typename R>
    concept input_range = range<R> and input_iterator<range_iter_t<R>>;

    template<typename R>
    concept output_range = range<R> and output_iterator<range_iter_t<R>>;

    template<typename R>
    concept forward_range = input_range<R> and output_range<R>;

    template<typename R>
    concept random_range = forward_range<R> and random_iterator<range_iter_t<R>>;

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

    template<typename T,size_t N>
    auto constexpr size(T (&a)[N]) -> size_t
    {
        return N;
    }

    template<typename R>
    requires requires(R r) {
        r.begin();
    }
    auto constexpr begin(R&& r) -> auto
    {
        return r.begin();
    }

    template<typename R>
    requires requires(R r) {
        r.end();
    }
    auto constexpr end(R&& r) -> auto
    {
        return r.end();
    }

    template<range R>
    requires requires(R r) {
        r.size();
    }
    auto constexpr size(R&& r) -> size_t
    {
        return r.size();
    }

    template<typename T,typename U>
    auto constexpr exchange(T& oldv,U&& newv) -> T
    {
        auto tmp = move(oldv);
        oldv = forward<U>(newv);
        return tmp;
    }

    template<typename T>
    auto constexpr swap(T& x,T& y) noexcept -> void
    {
        T tmp = move(x);
        x = move(y);
        y = move(tmp);
    }

    template<typename T>
    requires requires(T a,T b) { a.swap(b); }
    auto constexpr swap(T& x,T& y) noexcept -> void
    {
        x.swap(y);
    }

    template<typename T>
    auto constexpr max(T const& x,T const& y) -> T const&
    {
        if(not (x < y)) {
            return x;
        }
        return y;
    }

    template<typename T,typename... Ts>
    requires (std::same_as<T,Ts> and ...)
    auto constexpr max(T const& x,T const& y,Ts const&... other) -> T const&
    {
        return max(max(x,y),other...);
    }

    template<typename T>
    auto constexpr min(T const& x,T const& y) -> T const&
    {
        if(not (y < x)) {
            return x;
        }
        return y;
    }

    template<typename T,typename... Ts>
    requires (std::same_as<T,Ts> and ...)
    auto constexpr min(T const& x,T const& y,Ts const&... other) -> T const&
    {
        return min(min(x,y),other...);
    }


    template<integral T,T... Idx>
    struct integer_sequence
    {
        using value_type = T;
        auto static constexpr size() noexcept -> size_t
        { return sizeof...(Idx); }
    };

    template<integral T,T Num>
    using make_integer_sequence = integer_sequence<T,__integer_pack(Num)...>;

    template<size_t... Idx>
    using index_sequence = integer_sequence<size_t,Idx...>;

    template<size_t Num>
    using make_index_seqience = make_integer_sequence<size_t,Num>;

    template<typename... Args>
    using index_sequence_for = make_index_seqience<sizeof...(Args)>;

    template<signed_integral T>
    struct make_unsigned_s {};

    template<>
    struct make_unsigned_s<i8>
    {
        using type = u8;
    };

    template<>
    struct make_unsigned_s<i16>
    {
        using type = u16;
    };

    template<>
    struct make_unsigned_s<i32>
    {
        using type = u32;
    };

    template<signed_integral T>
    using make_unsigned = typename make_unsigned_s<T>::type;

    template<typename T>
    using decay = __decay(T);

    template<typename T>
    struct normalize_s
    {
        using type = decay<T>;
    };

    template<typename T>
    struct normalize_s<T*>
    {
        using type = decay<T>*;
    };

    template<typename T>
    using normalize = typename normalize_s<T>::type;

    using nullptr_t = decltype(nullptr);

    namespace iter
    {
        template<typename T>
        struct input
        {

            using value_type = T;

            auto constexpr operator*(this input const& self) -> T const&
            {
                return *self.it;
            }

            auto constexpr operator->() const -> T const*
            {
                return it;
            }

            auto constexpr operator++() -> input&
            {
                ++it;
                return *this;
            }

            auto friend constexpr operator==(input x,input y) -> bool
            {
                return x.it == y.it;
            }

            T* it;
        };

        template<typename T>
        struct output
        {

            using value_type = T;

            auto constexpr operator*(this auto&& self) -> decltype(auto)
            {
                return *self.it;
            }

            auto constexpr operator->(this auto&& self) -> decltype(auto)
            {
                return self.it;
            }

            auto constexpr operator++() -> output&
            {
                ++it;
                return *this;
            }

            auto friend operator==(output x,output y) -> bool
            {
                return x.it == y.it;
            }

            T* it;
        };

        template<typename T>
        struct forward
        {

            using value_type = T;

            auto constexpr operator*(this auto&& self) -> decltype(auto)
            {
                return *self.it;
            }

            auto constexpr operator++() -> forward&
            {
                ++it;
                return *this;
            }

            auto constexpr operator--() -> forward&
            {
                --it;
                return *this;
            }

            auto constexpr operator->(this auto&& self) -> decltype(auto)
            {
                return self.it;
            }

            auto friend constexpr operator==(forward x,forward y) -> bool
            {
                return x.it == y.it;
            }

            T* it;
        };

        template<typename T>
        struct random : forward<T>
        {

            using value_type = T;

            auto constexpr operator[](this auto&& self,size_t idx) -> decltype(auto)
            {
                return self.it[idx];
            }

            auto constexpr operator+=(i32 n) -> random&
            {
                this->it += n;
                return *this;
            }
        };
    }

    enum struct strong_ordering
    {
        less = -1,
        equal = 0,
        greater = 1
    };

    auto constexpr operator==(strong_ordering v,nullptr_t) noexcept -> bool
    {
        return v == strong_ordering::equal;
    }

    auto constexpr operator<(strong_ordering v,nullptr_t) noexcept -> bool
    {
        return v == strong_ordering::less;
    }

    auto constexpr operator>(strong_ordering v,nullptr_t) noexcept -> bool
    {
        return v == strong_ordering::greater;
    }

    auto constexpr operator<=(strong_ordering v,nullptr_t) noexcept -> bool
    {
        return v < 0 or v == 0;
    }

    auto constexpr operator>=(strong_ordering v,nullptr_t) noexcept -> bool
    {
        return not (v < 0);
    }

    auto constexpr operator==(strong_ordering x,strong_ordering y) noexcept -> bool
    {
        return x == y;
    }

    // NOLINTEND
}