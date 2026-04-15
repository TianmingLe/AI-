// /workspace/mocks/catch2/catch2/catch_test_macros.hpp
#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include <mutex>

namespace mock_catch2 {
    struct TestCase {
        std::string name;
        std::function<void()> func;
    };

    class TestRegistry {
    public:
        static TestRegistry& getInstance() {
            static TestRegistry instance;
            return instance;
        }

        void registerTest(const std::string& name, std::function<void()> func) {
            std::lock_guard<std::mutex> lock(mutex_);
            tests_.push_back({name, func});
        }

        int runAllTests() {
            int failed = 0;
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& test : tests_) {
                std::cout << "Running test: " << test.name << "\n";
                try {
                    test.func();
                    std::cout << "Passed test: " << test.name << "\n";
                } catch (const std::exception& e) {
                    std::cerr << "Failed test: " << test.name << " - " << e.what() << "\n";
                    failed++;
                } catch (...) {
                    std::cerr << "Failed test: " << test.name << " - Unknown exception\n";
                    failed++;
                }
            }
            return failed;
        }
    private:
        std::vector<TestCase> tests_;
        std::mutex mutex_;
    };
}

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define TEST_CASE(name, tags) \
    static void CONCAT(test_, __LINE__)(); \
    namespace { \
        struct CONCAT(TestReg_, __LINE__) { \
            CONCAT(TestReg_, __LINE__)() { \
                mock_catch2::TestRegistry::getInstance().registerTest(name, &CONCAT(test_, __LINE__)); \
            } \
        } CONCAT(test_reg_, __LINE__); \
    } \
    static void CONCAT(test_, __LINE__)()

#define REQUIRE(expr) \
    do { \
        if(!(expr)) { \
            std::cerr << "Requirement failed at " << __FILE__ << ":" << __LINE__ << "\n"; \
            throw std::runtime_error("Requirement failed"); \
        } \
    } while(0)

#define REQUIRE_FALSE(expr) REQUIRE(!(expr))
