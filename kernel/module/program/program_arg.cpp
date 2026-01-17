
import std.io;
import std.os;
import std.container;

auto main(int const argc, char* argv[]) -> int
{
    for(auto i = 0; i != argc; ++i) {
        std::println("argv[{}] is {}",i,argv[i]);
    }
    auto const pid = std::fork();
    if(pid) {
        auto delay = 900000;
        while(delay--) {

        }
        std::println();
        std::println (
            "I am father prog, my pid is {},"
            "I will show process list",
            std::getpid()
        );
        std::ps();
    } else {
        auto abspath = std::array<char,512>{};
        std::println();
        std::println (
            "I am a child prog, my pid is {},"
            "I will exec {} right now",
            std::getpid(),
            argv[1]
        );
        auto const exe = std::string_view{ argv[1] };
        if(exe.front() != '/') {
            std::getcwd(abspath.data(),512);
            auto buf = std::buffer{ abspath };
            buf += '/';
            buf += exe;
            std::exec(abspath.data(),argv);
        } else {
            std::exec(exe.data(),argv);
        }
    }
    while(true) {

    }
    return 0;
}