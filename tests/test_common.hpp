#ifndef FCASHER_TEST_COMMON_HPP
#define FCASHER_TEST_COMMON_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <windows.h>

namespace fcasher::tests {

struct TestCase {
    const char* name;
    bool (*run)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* name, bool (*run)()) {
        registry().push_back({name, run});
    }
};

inline std::string joinPath(const std::string& left, const std::string& right) {
    if (left.empty()) {
        return right;
    }
    if (left.back() == '\\') {
        return left + right;
    }
    return left + "\\" + right;
}

inline bool ensureDirectory(const std::string& path) {
    if (CreateDirectoryA(path.c_str(), nullptr) != 0) {
        return true;
    }
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

inline bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output << content;
    return output.good();
}

inline void removeTree(const std::string& root) {
    WIN32_FIND_DATAA data {};
    const std::string query = joinPath(root, "*");
    HANDLE handle = FindFirstFileA(query.c_str(), &data);
    if (handle != INVALID_HANDLE_VALUE) {
        do {
            if (std::string(data.cFileName) == "." || std::string(data.cFileName) == "..") {
                continue;
            }

            const std::string path = joinPath(root, data.cFileName);
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0U) {
                removeTree(path);
            } else {
                SetFileAttributesA(path.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFileA(path.c_str());
            }
        } while (FindNextFileA(handle, &data) != 0);
        FindClose(handle);
    }

    RemoveDirectoryA(root.c_str());
}

inline std::string makeTempDirectory(const std::string& leaf) {
    char buffer[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, buffer);
    const std::string root(buffer);
    const std::string path = joinPath(
        root,
        "fcasher_test_" + leaf + "_" + std::to_string(GetCurrentProcessId()) + "_" + std::to_string(GetTickCount()));
    ensureDirectory(path);
    return path;
}

}  // namespace fcasher::tests

#define FC_TEST(name)                                 \
    static bool name();                               \
    static fcasher::tests::Registrar reg_##name(#name, name); \
    static bool name()

#define FC_ASSERT(expr)                                                               \
    do {                                                                              \
        if (!(expr)) {                                                                \
            std::cerr << "Assertion failed: " #expr << " at " << __FILE__ << ':'      \
                      << __LINE__ << '\n';                                            \
            return false;                                                             \
        }                                                                             \
    } while (false)

#endif
