

export module array;

import utility;

export namespace std
{

    template<typename T,size_t N>
    struct array
    {

        using value_type = T;
        using iterator = T*;
        using const_iterator = T const*;

        auto constexpr operator[](this auto& self,size_t idx) -> auto&
        {
            return self.a[idx];
        }

        auto constexpr front(this auto& self) -> auto&
        {
            return self.a[0];
        }

        auto constexpr back(this auto& self) -> auto&
        {
            return self.a[N - 1];
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

}