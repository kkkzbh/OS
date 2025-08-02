module;

export module utility;


export template<typename T>
auto max(T const& x,T const& y) -> T const&
{
    if(y > x) {
        return y;
    }
    return x;
}