// /workspace/mocks/catch2/catch2/catch_test_macros.hpp
#pragma once
#include <iostream>

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define TEST_CASE(name, tags) \
    void CONCAT(test_, __LINE__)(); \
    struct CONCAT(TestReg_, __LINE__) { \
        CONCAT(TestReg_, __LINE__)() { \
            std::cout << "Running test: " << name << "\n"; \
            CONCAT(test_, __LINE__)(); \
            std::cout << "Passed test: " << name << "\n"; \
        } \
    } CONCAT(test_reg_, __LINE__); \
    void CONCAT(test_, __LINE__)()

#define REQUIRE(expr) if(!(expr)) { std::cerr << "Requirement failed at " << __FILE__ << ":" << __LINE__ << "\n"; exit(1); }
#define REQUIRE_FALSE(expr) REQUIRE(!(expr))
