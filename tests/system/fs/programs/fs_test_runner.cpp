import std;
import stat.structure;

namespace
{
    auto str_eq(char const* lhs, char const* rhs) -> bool
    {
        for(auto i = 0;; ++i) {
            if(lhs[i] != rhs[i]) {
                return false;
            }
            if(lhs[i] == '\0') {
                return true;
            }
        }
    }

    auto bytes_eq(char const* lhs, char const* rhs, int count) -> bool
    {
        for(auto i = 0; i != count; ++i) {
            if(lhs[i] != rhs[i]) {
                return false;
            }
        }
        return true;
    }

    auto fail(char const* case_name, char const* message) -> int
    {
        std::println("FSCASE:{}:FAIL", case_name);
        std::println("FSERR:{}:{}", case_name, message);
        return 1;
    }

    auto pass(char const* case_name) -> int
    {
        std::println("FSCASE:{}:PASS", case_name);
        return 0;
    }

    auto expect(bool condition, char const* case_name, char const* message) -> int
    {
        if(not condition) {
            return fail(case_name, message);
        }
        return 0;
    }

    auto cleanup_file(char const* path) -> void
    {
        std::unlink(path);
    }

    auto cleanup_dir(char const* path) -> void
    {
        std::rmdir(path);
    }

    auto case_file_basic() -> int
    {
        auto constexpr case_name = "file_basic";
        auto constexpr path = "/fs_basic.txt";
        cleanup_file(path);

        auto const fd = std::open(path, std::open_flags::create | std::open_flags::rdwr);
        if(auto rc = expect(fd != -1, case_name, "open create failed"); rc) {
            return rc;
        }

        auto constexpr payload = "alpha-beta";
        auto const written = std::write(fd, payload, 10);
        if(auto rc = expect(written == 10, case_name, "write length mismatch"); rc) {
            return rc;
        }
        if(auto rc = expect(std::close(fd), case_name, "close after write failed"); rc) {
            return rc;
        }

        auto const read_fd = std::open(path, std::open_flags::read);
        if(auto rc = expect(read_fd != -1, case_name, "reopen read failed"); rc) {
            return rc;
        }
        auto buf = std::array<char, 32>{};
        auto const read_bytes = std::read(read_fd, buf.data(), 10);
        if(auto rc = expect(read_bytes == 10, case_name, "read length mismatch"); rc) {
            return rc;
        }
        if(auto rc = expect(bytes_eq(buf.data(), payload, 10), case_name, "read data mismatch"); rc) {
            return rc;
        }
        if(auto rc = expect(std::close(read_fd), case_name, "close after read failed"); rc) {
            return rc;
        }

        cleanup_file(path);
        if(auto rc = expect(std::open(path, std::open_flags::read) == -1, case_name, "unlink did not remove file"); rc) {
            return rc;
        }
        return pass(case_name);
    }

    auto case_file_offset() -> int
    {
        auto constexpr case_name = "file_offset";
        auto constexpr path = "/fs_offset.txt";
        cleanup_file(path);

        auto const fd = std::open(path, std::open_flags::create | std::open_flags::rdwr);
        if(auto rc = expect(fd != -1, case_name, "open create failed"); rc) {
            return rc;
        }

        auto constexpr payload = "0123456789ABCDE";
        if(auto rc = expect(std::write(fd, payload, 15) == 15, case_name, "write payload failed"); rc) {
            return rc;
        }

        if(auto rc = expect(std::lseek(fd, 2, fs::whence::set) == 2, case_name, "lseek set failed"); rc) {
            return rc;
        }
        auto buf = std::array<char, 16>{};
        if(auto rc = expect(std::read(fd, buf.data(), 4) == 4, case_name, "read after set failed"); rc) {
            return rc;
        }
        if(auto rc = expect(bytes_eq(buf.data(), "2345", 4), case_name, "slice after set mismatch"); rc) {
            return rc;
        }

        if(auto rc = expect(std::lseek(fd, -3, fs::whence::end) == 12, case_name, "lseek end failed"); rc) {
            return rc;
        }
        buf = {};
        if(auto rc = expect(std::read(fd, buf.data(), 3) == 3, case_name, "read after end failed"); rc) {
            return rc;
        }
        if(auto rc = expect(bytes_eq(buf.data(), "CDE", 3), case_name, "tail slice mismatch"); rc) {
            return rc;
        }

        if(auto rc = expect(std::lseek(fd, -1, fs::whence::set) == -1, case_name, "negative lseek should fail"); rc) {
            return rc;
        }
        if(auto rc = expect(std::lseek(fd, 1, fs::whence::end) == -1, case_name, "past-end lseek should fail"); rc) {
            return rc;
        }
        if(auto rc = expect(std::close(fd), case_name, "close failed"); rc) {
            return rc;
        }

        cleanup_file(path);
        return pass(case_name);
    }

    auto case_directory_basic() -> int
    {
        auto constexpr case_name = "directory_basic";
        cleanup_file("/fs_dir_basic/inner/file");
        cleanup_dir("/fs_dir_basic/inner");
        cleanup_dir("/fs_dir_basic");

        if(auto rc = expect(std::mkdir("/fs_dir_basic"), case_name, "mkdir /fs_dir_basic failed"); rc) {
            return rc;
        }
        auto const dir = std::opendir("/fs_dir_basic");
        if(auto rc = expect(dir != nullptr, case_name, "opendir failed"); rc) {
            return rc;
        }
        if(auto rc = expect(std::closedir(dir), case_name, "closedir failed"); rc) {
            return rc;
        }
        if(auto rc = expect(std::mkdir("/fs_dir_basic/inner"), case_name, "mkdir nested failed"); rc) {
            return rc;
        }
        if(auto rc = expect(not std::rmdir("/fs_dir_basic"), case_name, "rmdir non-empty should fail"); rc) {
            return rc;
        }
        if(auto rc = expect(std::rmdir("/fs_dir_basic/inner"), case_name, "rmdir inner failed"); rc) {
            return rc;
        }
        if(auto rc = expect(std::rmdir("/fs_dir_basic"), case_name, "rmdir root dir failed"); rc) {
            return rc;
        }
        if(auto rc = expect(std::opendir("/fs_dir_basic") == nullptr, case_name, "removed dir should not open"); rc) {
            return rc;
        }
        return pass(case_name);
    }

    auto case_directory_iteration() -> int
    {
        auto constexpr case_name = "directory_iteration";
        cleanup_file("/fs_iter/a");
        cleanup_file("/fs_iter/b");
        cleanup_dir("/fs_iter");

        if(auto rc = expect(std::mkdir("/fs_iter"), case_name, "mkdir /fs_iter failed"); rc) {
            return rc;
        }
        auto fd = std::open("/fs_iter/a", std::open_flags::create | std::open_flags::rdwr);
        if(auto rc = expect(fd != -1, case_name, "create file a failed"); rc) {
            return rc;
        }
        std::close(fd);
        fd = std::open("/fs_iter/b", std::open_flags::create | std::open_flags::rdwr);
        if(auto rc = expect(fd != -1, case_name, "create file b failed"); rc) {
            return rc;
        }
        std::close(fd);

        auto dir = std::opendir("/fs_iter");
        if(auto rc = expect(dir != nullptr, case_name, "opendir iteration dir failed"); rc) {
            return rc;
        }

        auto saw_dot = false;
        auto saw_dotdot = false;
        auto saw_a = false;
        auto saw_b = false;
        auto first_count = 0;
        while(auto entry = std::readdir(dir)) {
            ++first_count;
            auto const name = entry->filename.data();
            saw_dot = saw_dot or str_eq(name, ".");
            saw_dotdot = saw_dotdot or str_eq(name, "..");
            saw_a = saw_a or str_eq(name, "a");
            saw_b = saw_b or str_eq(name, "b");
        }

        if(auto rc = expect(saw_dot and saw_dotdot and saw_a and saw_b, case_name, "readdir missing entries"); rc) {
            return rc;
        }

        std::rewinddir(dir);
        auto second_count = 0;
        while(std::readdir(dir)) {
            ++second_count;
        }
        if(auto rc = expect(first_count == second_count, case_name, "rewinddir count mismatch"); rc) {
            return rc;
        }
        if(auto rc = expect(std::closedir(dir), case_name, "closedir after iteration failed"); rc) {
            return rc;
        }

        cleanup_file("/fs_iter/a");
        cleanup_file("/fs_iter/b");
        cleanup_dir("/fs_iter");
        return pass(case_name);
    }

    auto case_metadata() -> int
    {
        auto constexpr case_name = "metadata";
        cleanup_file("/fs_meta/file");
        cleanup_dir("/fs_meta");

        if(auto rc = expect(std::mkdir("/fs_meta"), case_name, "mkdir /fs_meta failed"); rc) {
            return rc;
        }
        auto const fd = std::open("/fs_meta/file", std::open_flags::create | std::open_flags::rdwr);
        if(auto rc = expect(fd != -1, case_name, "create metadata file failed"); rc) {
            return rc;
        }
        if(auto rc = expect(std::write(fd, "hello", 5) == 5, case_name, "write metadata file failed"); rc) {
            return rc;
        }
        std::close(fd);

        auto root_stat = stat_t{};
        auto dir_stat = stat_t{};
        auto file_stat = stat_t{};
        if(auto rc = expect(std::stat("/", &root_stat), case_name, "stat / failed"); rc) {
            return rc;
        }
        if(auto rc = expect(root_stat.type == fs::file_type::directory, case_name, "root type mismatch"); rc) {
            return rc;
        }
        if(auto rc = expect(std::stat("/fs_meta", &dir_stat), case_name, "stat dir failed"); rc) {
            return rc;
        }
        if(auto rc = expect(dir_stat.type == fs::file_type::directory, case_name, "dir type mismatch"); rc) {
            return rc;
        }
        if(auto rc = expect(std::stat("/fs_meta/file", &file_stat), case_name, "stat file failed"); rc) {
            return rc;
        }
        if(auto rc = expect(file_stat.type == fs::file_type::regular, case_name, "file type mismatch"); rc) {
            return rc;
        }
        if(auto rc = expect(file_stat.size == 5, case_name, "file size mismatch"); rc) {
            return rc;
        }

        if(auto rc = expect(std::chdir("/fs_meta"), case_name, "chdir failed"); rc) {
            return rc;
        }
        auto cwd = std::array<char, 64>{};
        if(auto rc = expect(std::getcwd(cwd.data(), cwd.size()) != nullptr, case_name, "getcwd failed"); rc) {
            return rc;
        }
        if(auto rc = expect(str_eq(cwd.data(), "/fs_meta"), case_name, "cwd mismatch"); rc) {
            return rc;
        }
        std::chdir("/");

        cleanup_file("/fs_meta/file");
        cleanup_dir("/fs_meta");
        return pass(case_name);
    }

    auto case_error_paths() -> int
    {
        auto constexpr case_name = "error_paths";
        cleanup_file("/fs_err/file");
        cleanup_dir("/fs_err");

        if(auto rc = expect(std::open("/missing", std::open_flags::read) == -1, case_name, "missing file open should fail"); rc) {
            return rc;
        }
        if(auto rc = expect(not std::mkdir("/missing_parent/child"), case_name, "mkdir with missing parent should fail"); rc) {
            return rc;
        }
        if(auto rc = expect(std::mkdir("/fs_err"), case_name, "mkdir /fs_err failed"); rc) {
            return rc;
        }
        if(auto rc = expect(not std::mkdir("/fs_err"), case_name, "duplicate mkdir should fail"); rc) {
            return rc;
        }
        if(auto rc = expect(std::open("/fs_err", std::open_flags::read) == -1, case_name, "open dir as file should fail"); rc) {
            return rc;
        }
        auto const file_fd = std::open("/fs_err/file", std::open_flags::create | std::open_flags::read);
        if(auto rc = expect(file_fd != -1, case_name, "create file in error case failed"); rc) {
            return rc;
        }
        if(auto rc = expect(std::write(file_fd, "z", 1) == -1, case_name, "write on read-only fd should fail"); rc) {
            return rc;
        }
        std::close(file_fd);
        if(auto rc = expect(not std::rmdir("/fs_err"), case_name, "rmdir non-empty should fail"); rc) {
            return rc;
        }
        if(auto rc = expect(not std::unlink("/fs_err"), case_name, "unlink directory should fail"); rc) {
            return rc;
        }
        if(auto rc = expect(not std::chdir("/fs_err/file"), case_name, "chdir regular file should fail"); rc) {
            return rc;
        }

        cleanup_file("/fs_err/file");
        cleanup_dir("/fs_err");
        return pass(case_name);
    }

    auto case_persistence_file_write() -> int
    {
        auto constexpr case_name = "persistence_file_write";
        auto constexpr path = "/persist_file.txt";
        cleanup_file(path);
        auto const fd = std::open(path, std::open_flags::create | std::open_flags::rdwr);
        if(auto rc = expect(fd != -1, case_name, "open persistence file failed"); rc) {
            return rc;
        }
        auto constexpr payload = "PERSIST-FILE-OK";
        if(auto rc = expect(std::write(fd, payload, 15) == 15, case_name, "write persistence file failed"); rc) {
            return rc;
        }
        std::close(fd);
        auto st = stat_t{};
        if(auto rc = expect(std::stat(path, &st), case_name, "stat persistence file failed"); rc) {
            return rc;
        }
        if(auto rc = expect(st.size == 15, case_name, "persistence file size mismatch"); rc) {
            return rc;
        }
        return pass(case_name);
    }

    auto case_persistence_file_verify() -> int
    {
        auto constexpr case_name = "persistence_file_verify";
        auto constexpr path = "/persist_file.txt";
        auto const fd = std::open(path, std::open_flags::read);
        if(auto rc = expect(fd != -1, case_name, "open persisted file failed"); rc) {
            return rc;
        }
        auto buf = std::array<char, 32>{};
        if(auto rc = expect(std::read(fd, buf.data(), 15) == 15, case_name, "read persisted file failed"); rc) {
            return rc;
        }
        std::close(fd);
        if(auto rc = expect(bytes_eq(buf.data(), "PERSIST-FILE-OK", 15), case_name, "persisted file mismatch"); rc) {
            return rc;
        }
        return pass(case_name);
    }

    auto case_persistence_directory_write() -> int
    {
        auto constexpr case_name = "persistence_directory_write";
        cleanup_file("/persist_dir/sub/file");
        cleanup_dir("/persist_dir/sub");
        cleanup_dir("/persist_dir");

        if(auto rc = expect(std::mkdir("/persist_dir"), case_name, "mkdir persist_dir failed"); rc) {
            return rc;
        }
        if(auto rc = expect(std::mkdir("/persist_dir/sub"), case_name, "mkdir persist_dir/sub failed"); rc) {
            return rc;
        }
        auto const fd = std::open("/persist_dir/sub/file", std::open_flags::create | std::open_flags::rdwr);
        if(auto rc = expect(fd != -1, case_name, "create persisted file failed"); rc) {
            return rc;
        }
        if(auto rc = expect(std::write(fd, "dir-ok", 6) == 6, case_name, "write persisted dir file failed"); rc) {
            return rc;
        }
        std::close(fd);
        return pass(case_name);
    }

    auto case_persistence_directory_verify() -> int
    {
        auto constexpr case_name = "persistence_directory_verify";
        auto dir = std::opendir("/persist_dir/sub");
        if(auto rc = expect(dir != nullptr, case_name, "opendir persisted dir failed"); rc) {
            return rc;
        }
        auto saw_file = false;
        while(auto entry = std::readdir(dir)) {
            saw_file = saw_file or str_eq(entry->filename.data(), "file");
        }
        std::closedir(dir);
        if(auto rc = expect(saw_file, case_name, "persisted directory missing file"); rc) {
            return rc;
        }
        auto st = stat_t{};
        if(auto rc = expect(std::stat("/persist_dir/sub/file", &st), case_name, "stat persisted file failed"); rc) {
            return rc;
        }
        if(auto rc = expect(st.type == fs::file_type::regular, case_name, "persisted file type mismatch"); rc) {
            return rc;
        }
        return pass(case_name);
    }
}

auto main(int argc, char* argv[]) -> int
{
    if(argc != 2) {
        return fail("usage", "expected exactly one case name");
    }

    auto const case_name = std::string_view{ argv[1] };
    if(case_name == "file_basic") {
        return case_file_basic();
    }
    if(case_name == "file_offset") {
        return case_file_offset();
    }
    if(case_name == "directory_basic") {
        return case_directory_basic();
    }
    if(case_name == "directory_iteration") {
        return case_directory_iteration();
    }
    if(case_name == "metadata") {
        return case_metadata();
    }
    if(case_name == "error_paths") {
        return case_error_paths();
    }
    if(case_name == "persistence_file_write") {
        return case_persistence_file_write();
    }
    if(case_name == "persistence_file_verify") {
        return case_persistence_file_verify();
    }
    if(case_name == "persistence_directory_write") {
        return case_persistence_directory_write();
    }
    if(case_name == "persistence_directory_verify") {
        return case_persistence_directory_verify();
    }

    return fail("dispatch", "unknown case");
}
