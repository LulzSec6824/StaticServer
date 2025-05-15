#include "../include/config.h"
#include "test_utils.hpp"
#include <iostream>
#include <string>

// Test default configuration values
void test_default_config() {
    ServerConfig config;
    test_utils::test_assert(config.port == 8080, "Default port should be 8080");
    test_utils::test_assert(config.root_directory == "./public",
                            "Default root directory should be ./public");
}

// Test custom configuration values
void test_custom_config() {
    ServerConfig config;

    // Test setting port
    config.port = 9090;
    test_utils::test_assert(config.port == 9090,
                            "Port should be updated to 9090");

    // Test setting root directory
    config.root_directory = "/var/www";
    test_utils::test_assert(config.root_directory == "/var/www",
                            "Root directory should be updated to /var/www");
}

// Test configuration edge cases
void test_config_edge_cases() {
    ServerConfig config;

    // Test negative port (should generally be caught elsewhere)
    config.port = -1;
    test_utils::test_assert(config.port == -1,
                            "Port should allow any integer value");

    // Test empty root directory
    config.root_directory = "";
    test_utils::test_assert(config.root_directory == "",
                            "Root directory should allow empty string");
}

int main() {
    std::cout << "===== Running ServerConfig Tests =====" << std::endl;

    test_utils::run_test("Default Configuration", test_default_config);
    test_utils::run_test("Custom Configuration", test_custom_config);
    test_utils::run_test("Configuration Edge Cases", test_config_edge_cases);

    test_utils::print_test_summary();

    return 0;
}