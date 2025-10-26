

export module vector;

import algorithm;
import utility;

export namespace std
{
    template<typename T>
    struct vector
    {
        using value_type = T;
        using iterator = std::iter::random<T>;

        auto static constexpr epf = size_t{ 2 };

        vector() = default;

        explicit vector(integral auto n) : a(new T[n]{}),bound(a + n),it(bound) {}

        vector(integral auto n,T const& value) : vector(n)
        {
            *this | fill[value];
        }

        auto operator=(vector const& other) -> vector&
        {
            resize(other.size());
            return *this | copy[other];
        }

        auto operator=(vector&& other) noexcept -> vector&
        {
            std::swap(a,other.a);
            std::swap(it,other.it);
            std::swap(bound,other.bound);
            return *this;
        }

        vector(vector const& other) : vector(other.size())
        {
            *this | copy[other];
        }

        vector(vector&& other) noexcept : a(other.a), bound(other.bound), it(other.it)
        {
            other.a = nullptr;
            other.bound = nullptr;
            other.it = nullptr;
        }

        explicit constexpr operator bool() const
        {
            return bool(a);
        }

        template<random_range R>
        vector(R&& r) : vector(std::size(r))
        {
            *this | copy[forward<R>(r)];
        }

        // ~vector() noexcept
        // {
        //     delete[] a;
        // }

        auto realloc(size_t n) -> void
        {
            auto na = vector(n);
            na | copy[*this];
            delete[] a;
            *this = move(na);
        }

        auto reserve(size_t n) -> void
        {
            if(capacity() >= n) {
                return;
            }
            realloc(n);
        }

        auto resize(size_t n) -> void
        {
            if(n > capacity()) {
                realloc(n);
            }
            it = a + n;
        }

        template<random_range R>
        auto operator=(R&& other) -> vector&
        {
            resize(std::size(other));
            *this | copy[forward<R>(other)];
            return *this;
        }

        auto operator[](this auto&& self,size_t idx) -> decltype(auto)
        {
            return self.a[idx];
        }

        auto front(this auto&& self) -> decltype(auto)
        {
            return self.a[0];
        }

        auto back(this auto&& self) -> decltype(auto)
        {
            return self.it[-1];
        }

        auto data(this auto&& self) -> decltype(auto)
        {
            return self.a;
        }

        auto begin(this auto&& self) -> decltype(auto)
        {
            return self.a;
        }

        auto end(this auto&& self) -> decltype(auto)
        {
            return self.it;
        }

        [[nodiscard]]
        auto size() const -> size_t
        {
            return it - a;
        }

        [[nodiscard]]
        auto capacity() const -> size_t
        {
            return bound - a;
        }

        [[nodiscard]]
        auto empty() const -> bool
        {
            return size() == 0;
        }

        auto shrink() -> void
        {
            realloc(size());
        }

        auto clear() -> void
        {
            it = a;
        }

        template<typename... Args>
        auto emplace_back(Args&&... args) -> void
        {
            if(it == bound) [[unlikely]] {
                realloc(epf * size());
            }
            *it++ = T{ forward<Args>(args)... };
        }

        template<random_range R>
        auto emplace_back(R&& r) -> void
        {
            if(capacity() - size() < size(r)) [[unlikely]] {
                realloc(epf * (size() + size(r)));
            }
            subrange(it,it + size(r)) | copy[forward<R>(r)];
            it += size(r);
        }

        auto pop_back() -> void
        {
            --it;
        }

        auto erase(size_t idx) -> void
        {
            for(auto i = a + idx + 1; i != it; ++i) {
                *(i - 1) = *i;
            }
            --it;
        }

        auto operator[](this auto&& self,size_t x,size_t y) -> decltype(auto)
        {
            return subrange(self.a + x,self.a + y);
        }

        auto erase(size_t x,size_t y) -> void
        {
            erase(subrange(a + x,a + y));
        }

        template<typename It,typename Sentry>
        auto erase(subrange<It,Sentry> sr) -> void
        {
            auto [l,r] = sr;
            while(r != it) {
                *l++ = *r++;
            }
            it -= r - l;
        }

        T* a = nullptr;
        T* bound = nullptr;
        T* it = nullptr;
    };

    struct tovec_fn
    {} constexpr tovec;

    template<typename R>
    auto operator|(R&& r,tovec_fn) -> vector<range_value_t<R>>
    {
        return r;
    }

}