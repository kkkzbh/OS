

export module algorithm;

import utility;

namespace std
{

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
            [f = move(func),&args...]<typename T>(T&& v) mutable -> decltype(auto) {
                return f(forward<T>(v),forward<Args>(args)...);
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

    export struct copy_fn
    {
        template<typename R,typename R2>
        auto static constexpr operator()(R&& r,R2&& r2) -> R&&
        {
            auto bound = min(size(r),size(r2));
            auto it1 = begin(r);
            auto it2 = begin(r2);
            for(auto i = 0; i != bound; ++i) {
                *it1 = *it2;
                ++it1;
                ++it2;
            }
            return forward<R>(r);
        }

        template<typename R2>
        auto constexpr operator[](this auto self,R2&& r2) -> decltype(auto)
        {
            return bind(self,forward<R2>(r2));
        }
    } constexpr copy;

    export template<typename It,typename Sentry>
    struct subrange
    {

        using value_type = iter_value_t<It>;
        using iterator = It;
        using const_iterator = It const;

        constexpr subrange(It it,Sentry sentry) : it(it), sentry(sentry) {}

        auto constexpr begin() const -> It
        {
            return it;
        }

        auto constexpr end() const -> Sentry
        {
            return sentry;
        }

        It it;
        Sentry sentry;
    };

}