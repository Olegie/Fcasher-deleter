#include <iostream>

#include "test_common.hpp"

int main() {
    int failures = 0;

    for (const auto& test : fcasher::tests::registry()) {
        if (test.run()) {
            std::cout << "[PASS] " << test.name << '\n';
        } else {
            std::cout << "[FAIL] " << test.name << '\n';
            ++failures;
        }
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All tests passed.\n";
    return 0;
}
