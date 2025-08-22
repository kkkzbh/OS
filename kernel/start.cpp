


void init_all();
auto main() -> void;


extern "C" auto start() -> void
{
    init_all();
    main();
    while(true) {

    }
}