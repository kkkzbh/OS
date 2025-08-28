

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

    // auto r = a | first[([](auto v) {
    //     return v % 2 == 0;
    // })];
    //
    // auto k = r.get();
    // println("k = {}",k);

    a[1] = 999;

    println("a[1] = {}",a[1]);

}