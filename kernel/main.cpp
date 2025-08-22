

export module main;

import kernel;

auto main() -> void
{
    auto a = std::array{ 1,2,3,4,5 };
    console::println("Hella world");
    console::println("The array is {}",a);
}