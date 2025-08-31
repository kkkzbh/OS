

export module array;

import utility;
import algorithm;

export namespace std
{

    template<typename T,size_t N>
    struct array
    {

        using value_type = T;
        using iterator = std::iter::random<T>;

        auto constexpr operator[](this auto&& self,size_t idx) -> decltype(auto)
        {
            return (self.a[idx]);
        }

        auto constexpr operator[](size_t x,size_t y) -> decltype(auto)
        {
            return subrange{ a + x,a + y };
        }

        auto constexpr front(this auto& self) -> auto&
        {
            return (self.a[0]);
        }

        auto constexpr back(this auto& self) -> auto&
        {
            return (self.a[N - 1]);
        }

        auto constexpr data(this auto& self) -> auto*
        {
            return self.a;
        }

        auto constexpr begin(this auto& self) -> auto*
        {
            return std::begin(self.a);
        }

        auto constexpr end(this auto& self) -> auto*
        {
            return std::end(self.a);
        }

        auto static constexpr size() -> size_t
        {
            return N;
        }

        auto constexpr empty() const -> bool
        {
            return begin() == end();
        }

        T a[N];
    };

    template<typename... Args>
    array(Args... args) -> array<Args...[0],sizeof...(args)>;

    // 与上述等价，但为了兼容目前的IDE还无法识别C++26形参包语法，故补充旧语法
    template<typename T,typename... Args>
    array(T arg,Args... args) -> array<T,1 + sizeof...(args)>;

}