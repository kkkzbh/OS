

export module os;

import kernel;
import test.filesystem;

auto func() -> void
{
    auto a = std::vector(50000,int{});
    for(auto i = 0; i != a.size(); ++i) {
        a[i] = i;
    }
    auto fd = open("/test_f",+open_flags::create | +open_flags::write);
    write(fd,a.data(),a.size());
    close(fd);
    fd = open("/test_f",+open_flags::read);
    auto b = std::vector(50000,int{});
    read(fd,b.data(),b.size());
    for(auto i = 0; i != 20; ++i) {
        console::print("{} ",b[i]);
    }
    console::println();
}

auto main() -> void
{
    func();
}