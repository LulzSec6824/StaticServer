#include "../include/file_utils.h"
#include "test_utils.hpp"
#include <iostream>
#include <string>

// Test file existence function
void test_file_exists() {
    const std::string TEST_FILE = "test_temp_file.txt";
    const std::string TEST_CONTENT = "This is test content";

    // Create test file
    bool created = test_utils::create_test_file(TEST_FILE, TEST_CONTENT);
    test_utils::test_assert(created, "Failed to create test file");

    // Test file exists
    bool exists = file_utils::file_exists(TEST_FILE);
    test_utils::test_assert(
        exists, "file_exists() should return true for existing file");

    // Test non-existent file
    bool not_exists = file_utils::file_exists("non_existent_file.xyz");
    test_utils::test_assert(
        !not_exists, "file_exists() should return false for non-existent file");

    // Clean up
    test_utils::cleanup_test_file(TEST_FILE);
}

// Test file reading function
void test_read_file() {
    const std::string TEST_FILE = "test_temp_file.txt";
    const std::string TEST_CONTENT = "This is test content";

    // Create test file
    test_utils::create_test_file(TEST_FILE, TEST_CONTENT);

    // Test reading file content
    std::string content = file_utils::read_file(TEST_FILE);
    test_utils::test_assert(content == TEST_CONTENT,
                            "read_file() should return correct file content");

    // Clean up
    test_utils::cleanup_test_file(TEST_FILE);
}

// Test empty file reading
void test_read_empty_file() {
    const std::string EMPTY_FILE = "empty_file.txt";

    // Create empty file
    test_utils::create_test_file(EMPTY_FILE, "");

    // Test reading empty file
    std::string empty_content = file_utils::read_file(EMPTY_FILE);
    test_utils::test_assert(
        empty_content.empty(),
        "read_file() should return empty string for empty file");

    // Clean up
    test_utils::cleanup_test_file(EMPTY_FILE);
}

// Test reading large file
void test_read_large_file() {
    const std::string LARGE_FILE = "large_file.txt";
    std::string large_content(10 * 1024, 'A'); // 10KB of 'A' characters

    // Create large file
    test_utils::create_test_file(LARGE_FILE, large_content);

    // Test reading large file
    std::string read_large = file_utils::read_file(LARGE_FILE);
    test_utils::test_assert(read_large.size() == large_content.size(),
                            "read_file() should correctly read large files");

    // Clean up
    test_utils::cleanup_test_file(LARGE_FILE);
}

// Test file extension extraction
void test_file_extension() {
    // Test various file extensions
    test_utils::test_assert(file_utils::get_file_extension("test.txt") ==
                                ".txt",
                            "get_file_extension() should return .txt");
    test_utils::test_assert(file_utils::get_file_extension("index.html") ==
                                ".html",
                            "get_file_extension() should return .html");
    test_utils::test_assert(file_utils::get_file_extension("style.css") ==
                                ".css",
                            "get_file_extension() should return .css");
    test_utils::test_assert(file_utils::get_file_extension("script.js") ==
                                ".js",
                            "get_file_extension() should return .js");

    // Test cases with no extension
    test_utils::test_assert(file_utils::get_file_extension("README") == "",
                            "get_file_extension() should return empty string "
                            "for files without extension");
    test_utils::test_assert(file_utils::get_file_extension("noextension") == "",
                            "get_file_extension() should return empty string "
                            "for files without extension");

    // Test edge cases
    test_utils::test_assert(
        file_utils::get_file_extension(".gitignore") == ".gitignore",
        "get_file_extension() should handle filenames starting with dot");
    test_utils::test_assert(
        file_utils::get_file_extension("file.with.multiple.dots") == ".dots",
        "get_file_extension() should return only the part after the last dot");
    test_utils::test_assert(file_utils::get_file_extension("") == "",
                            "get_file_extension() should handle empty strings");
}

// Test error handling for non-existent files
void test_read_nonexistent_file() {
    bool exception_caught = false;

    try {
        file_utils::read_file("non_existent_file.xyz");
    } catch (const std::runtime_error &) {
        exception_caught = true;
    }

    test_utils::test_assert(
        exception_caught,
        "read_file() should throw exception for non-existent file");
}

int main() {
    std::cout << "===== Running File Utils Tests =====" << std::endl;

    test_utils::run_test("File Existence", test_file_exists);
    test_utils::run_test("File Reading", test_read_file);
    test_utils::run_test("Empty File Reading", test_read_empty_file);
    test_utils::run_test("Large File Reading", test_read_large_file);
    test_utils::run_test("File Extension", test_file_extension);
    test_utils::run_test("Non-existent File Reading",
                         test_read_nonexistent_file);

    test_utils::print_test_summary();

    return 0;
}