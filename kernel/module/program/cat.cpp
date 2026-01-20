

import std.io;
import std.os;
import std.container;

auto main(int argc,char* argv[]) -> int
{
    if(argc > 2 or argc == 1) {
        std::println("cat: only support 1 argument.");
        std::println("eg: cat filename");
        std::exit(-2);
    }
    std::array<char,512> abs_path{};
    std::vector buf(512,char{});
    if(not buf) {
        std::println("cat: malloc memory failed!");
        return -1;
    }
    std::string_view filename{ argv[1] };
    std::buffer a{ abs_path.data() };
    if(filename.front() != '/') {
        std::getcwd(abs_path.data(),abs_path.size());
        a += '/';
        a += filename;
    } else {
        a += filename;
    }
    auto const fd = std::open(abs_path.data(),std::open_flags::read);
    if(fd == -1) {
        std::println("cat: open {} failed!",filename);
        return -1;
    }
    while(true) {
        auto read_bytes = std::read(fd,buf.data(),buf.size());
        if(read_bytes == -1) {
            break;
        }
        std::write(1,buf.data(),read_bytes);
    }
    std::close(fd);
    return 66;
}