// /workspace/mocks/catch2/mock_main.cpp
#include <iostream>
#include <catch2/catch_test_macros.hpp>

int main() {
    std::cout << "Mock Catch2 tests running...\n";
    int failed = mock_catch2::TestRegistry::getInstance().runAllTests();
    if (failed > 0) {
        std::cerr << failed << " tests failed!\n";
        return 1;
    }
    std::cout << "All tests passed!\n";
    return 0;
}
