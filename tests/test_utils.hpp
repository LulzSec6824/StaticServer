#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_utils {

// Test result tracking
struct TestResult {
    std::string test_name;
    bool passed;
    std::string message;
};

static std::vector<TestResult> test_results;

/**
 * Run a test function and track the result
 */
inline void run_test(const std::string &test_name,
                     std::function<void()> test_func) {
    std::cout << "Running test: " << test_name << "..." << std::endl;
    try {
        test_func();
        test_results.push_back({test_name, true, "PASS"});
        std::cout << "\033[1;32m[PASS]\033[0m " << test_name << std::endl;
    } catch (const std::exception &e) {
        test_results.push_back({test_name, false, e.what()});
        std::cout << "\033[1;31m[FAIL]\033[0m " << test_name << ": " << e.what()
                  << std::endl;
    } catch (...) {
        test_results.push_back({test_name, false, "Unknown error"});
        std::cout << "\033[1;31m[FAIL]\033[0m " << test_name
                  << ": Unknown error" << std::endl;
    }
}

/**
 * Print a summary of all test results
 */
inline void print_test_summary() {
    int passed = 0;
    int failed = 0;

    std::cout << "\n===== Test Results Summary =====\n";

    for (const auto &result : test_results) {
        if (result.passed) {
            passed++;
        } else {
            failed++;
            std::cout << "\033[1;31m[FAIL]\033[0m " << result.test_name << ": "
                      << result.message << std::endl;
        }
    }

    std::cout << "\nTotal tests: " << test_results.size() << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    if (failed == 0) {
        std::cout << "\033[1;32m===== All Tests Passed! =====\033[0m\n";
    } else {
        std::cout << "\033[1;31m===== Some Tests Failed =====\033[0m\n";
    }
}

/**
 * Create a test file with specified content
 *
 * @param path The path where to create the file
 * @param content The content to write to the file
 * @return bool True if creation succeeded
 */
inline bool create_test_file(const std::string &path,
                             const std::string &content) {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        file << content;
        file.close();
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Error creating test file: " << e.what() << std::endl;
        return false;
    }
}

/**
 * Clean up a test file
 *
 * @param path The path to the file to remove
 * @return bool True if removal succeeded
 */
inline bool cleanup_test_file(const std::string &path) {
    return std::remove(path.c_str()) == 0;
}

/**
 * Create a directory if it doesn't exist
 *
 * @param path The directory path to create
 * @return bool True if creation succeeded or directory already exists
 */
inline bool ensure_directory(const std::string &path) {
    return system(("mkdir -p " + path).c_str()) == 0;
}

/**
 * Custom assert that throws an exception with message on failure
 */
inline void test_assert(bool condition,
                        const std::string &message = "Assertion failed") {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace test_utils

#endif // TEST_UTILS_HPP