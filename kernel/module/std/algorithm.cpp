

export module algorithm;

import utility;

namespace std
{
    export template<typename T>
    auto constexpr swap(T& x,T& y) noexcept -> void
    {
        T tmp = move(x);
        x = move(y);
        y = move(tmp);
    }

    export template<typename T>
    auto constexpr max(T const& x,T const& y) -> T const&
    {
        if(y > x) {
            return y;
        }
        return x;
    }

    template<typename Lambda>
    struct bind_function
    {
        template<typename T>
        auto operator()(T&& v) -> decltype(auto)
        {
            return func(forward<T>(v));
        }

        Lambda func;
    };

    template<typename Func,typename... Args>
    auto constexpr bind(Func func,Args&&... args) -> auto
    {
        return bind_function {
            [f = move(func),...args = forward<Args>(args)]<typename T>(T&& v) mutable -> decltype(auto) {
                return f(forward<T>(v),args...);
            }
        };
    }

    export template<typename Lhs,typename Lam>
    auto constexpr operator|(Lhs&& lhs,bind_function<Lam> rhs) -> decltype(auto)
    {
        return rhs(lhs);
    }

    export struct for_each_fn
    {
        template<typename R,typename Func>
        auto static constexpr operator()(R&& r,Func func) -> R&&
        {
            for(auto& v : r) {
                func(v);
            }
            return forward<R>(r);
        }

        template<typename Func>
        auto constexpr operator[](this auto self,Func func) -> decltype(auto)
        {
            return bind(self,func);
        }

    } constexpr for_each;

    export struct fill_fn
    {
        template<typename R>
        auto static constexpr operator()(R&& r,range_value_t<R> const& val) -> R&&
        {
            for(auto& v : r) {
                v = val;
            }
            return forward<R>(r);
        }

        template<typename T>
        auto constexpr operator[](this auto self,T const& val) -> decltype(auto)
        {
            return bind(self,val);
        }

    } constexpr fill;

}