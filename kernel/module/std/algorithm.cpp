

export module algorithm;

import utility;
import optional;
import reference;

namespace std
{

    export template<typename It,typename Sentry>
    struct subrange
    {

        using value_type = iter_value_t<It>;
        using iterator = It;

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
    auto constexpr bind(Func func,Args const&... args) -> auto
    {
        return bind_function {
            [f = move(func),&args...]<typename T>(T&& v) mutable -> decltype(auto) {
                return f(forward<T>(v),args...);
            }
        };
    }

    export template<typename Lhs,typename Lam>
    auto constexpr operator|(Lhs&& lhs,bind_function<Lam>&& rhs) -> decltype(auto)
    {
        return rhs(forward<Lhs>(lhs));
    }

    export struct apply_fn
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
        auto constexpr operator[](this auto self,Func const& func) -> decltype(auto)
        {
            return bind(self,func);
        }

    } constexpr apply;

    export struct apply_until_fn
    {
        template<typename R,typename Func>
        auto static constexpr operator()(R&& r,Func func) -> R&&
        {
            for(auto& v : r) {
                if(func(v)) {
                    break;
                }
            }
            return forward<R>(r);
        }

        template<typename Func>
        auto constexpr operator[](this auto self,Func const& func) -> decltype(auto)
        {
            return bind(self,func);
        }

    } constexpr apply_until;

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
        auto static constexpr operator()(R&& r,R2 const& r2) -> R&&
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
        auto constexpr operator[](this auto self,R2 const& r2) -> decltype(auto)
        {
            return bind(self,r2);
        }
    } constexpr copy;

    export struct first_fn
    {
        template<typename R>
        auto static constexpr operator()(R&& r,range_value_t<R> const& r2) -> reference<range_value_t<R>>
        {
            using return_type = reference<decltype(*begin(r))&&>;
            for(auto&& it : r) {
                if(it == r2) {
                    return return_type{ forward<decltype(it)>(it) };
                }
            }
            return return_type{};
        }

        template<typename R,typename Pred>
        requires requires(Pred pred,range_value_t<R> v) { pred(v); }
        auto static constexpr operator()(R&& r,Pred pred) -> decltype(auto)
        {
            using return_type = reference<decltype(*begin(r))&&>;
            for(auto&& it : r) {
                if(pred(it)) {
                    return return_type{ forward<decltype(it)>(it) };
                }
            }
            return return_type{};
        }

        template<typename V_Pred>
        auto constexpr operator[](this auto self,V_Pred const& v_or_pred) -> decltype(auto)
        {
            return bind(self,v_or_pred);
        }

    } constexpr first;

    export struct decorate_fn
    {

        template<typename R,typename Pred>
        struct range
        {
            R&& r;
            Pred const& pred;

            struct iter : range_iter_t<R>
            {
                using value_type = decltype((*(Pred*){})(*(range_value_t<R>*){}));

                auto constexpr operator*() -> decltype(auto)
                {
                    return pred(*this->it);
                }
                Pred const& pred;

            };


            auto constexpr begin() -> iter
            {
                return { std::begin(r),pred };
            }

            auto constexpr end() -> iter
            {
                return { std::end(r),pred };
            }

            using value_type = iter_value_t<iter>;
            using iterator = iter;

        };

        template<typename R,typename Pred>
        range(R&& r,Pred pred) -> range<R,Pred>;

        template<typename R,typename Pred>
        auto constexpr operator()(R&& r,Pred const& pred) -> decltype(auto)
        {
            return range{ forward<R>(r),pred };
        }

        template<typename Pred>
        auto constexpr operator[](this auto self,Pred const& pred) -> decltype(auto)
        {
            return bind(self,pred);
        }

    } constexpr decorate;



}