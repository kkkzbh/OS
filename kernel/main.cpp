

export module main;

import kernel;

using namespace std;
using namespace console;

auto main() -> void
{


    auto a = std::vector<int>{};
    a.emplace_back(1);
    a.emplace_back(4);
    a.emplace_back(7);

    auto r = a | decorate[([](auto v) {
        return v * 2;
    })] | first[([](auto v) {
        return v % 2 == 0;
    })];


    println("{}",a);

    static_assert(std::same_as<decltype(*r.it),int&>);

    auto y = r.get();

    println("{}",a);



}