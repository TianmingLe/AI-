// /workspace/mocks/catch2/mock_main.cpp
#include <iostream>

void run_all_tests(); // To be implemented by mock macros if needed, or just let static initialization run them

int main() {
    std::cout << "Mock Catch2 tests running...\n";
    return 0;
}
