#include "../include/config.h"
#include "../include/server.h"
#include "test_utils.hpp"
#include <iostream>
#include <string>

// Mock class to test StaticFileServer functionality
class TestableStaticFileServer : public StaticFileServer {
  public:
    TestableStaticFileServer(const ServerConfig &config)
        : StaticFileServer(config) {}

    // Expose private methods for testing
    bool test_initialize_socket() {
        try {
            initialize_socket();
            return true;
        } catch (const std::exception &e) {
            std::cerr << "Socket initialization error: " << e.what()
                      << std::endl;
            return false;
        }
    }

    std::string test_get_content_type(const std::string &path) {
        return get_content_type(path);
    }

    void test_initialize_mime_types() { 
        initialize_mime_types(); 
    }
};
// Test server initialization
void test_server_init() {
    ServerConfig config;
    config.port = 8081; // Use a different port than default to avoid conflicts

    try {
        TestableStaticFileServer server(config);
        bool initialized = server.test_initialize_socket();
        test_utils::test_assert(initialized,
                                "Server socket initialization failed");
    } catch (const std::exception &e) {
        test_utils::test_assert(
            false, std::string("Server initialization exception: ") + e.what());
    }
}

// Test MIME type detection
void test_mime_types() {
    ServerConfig config;
    TestableStaticFileServer server(config);

    // Test common MIME types
    test_utils::test_assert(server.test_get_content_type("test.html") ==
                                "text/html",
                            "HTML content type should be text/html");
    test_utils::test_assert(server.test_get_content_type("styles.css") ==
                                "text/css",
                            "CSS content type should be text/css");
    test_utils::test_assert(server.test_get_content_type("script.js") ==
                                "application/javascript",
                            "JS content type should be application/javascript");
    test_utils::test_assert(server.test_get_content_type("data.json") ==
                                "application/json",
                            "JSON content type should be application/json");
    test_utils::test_assert(server.test_get_content_type("image.png") ==
                                "image/png",
                            "PNG content type should be image/png");

    // Test unknown type
    test_utils::test_assert(
        server.test_get_content_type("unknown.xyz") ==
            "application/octet-stream",
        "Unknown content type should default to application/octet-stream");
}

// Test server configuration
void test_server_config() {
    // Test with custom port
    {
        ServerConfig config;
        config.port = 9999;
        TestableStaticFileServer server(config);
        // If we get here without exception, config was accepted
        test_utils::test_assert(true, "Server should accept custom port");
    }

    // Test with custom root directory
    {
        ServerConfig config;
        config.root_directory = "./custom_public";
        test_utils::ensure_directory(config.root_directory);

        TestableStaticFileServer server(config);
        // If we get here without exception, config was accepted
        test_utils::test_assert(true,
                                "Server should accept custom root directory");

        // Clean up
        system(("rmdir " + config.root_directory).c_str());
    }
}

int main() {
    std::cout << "===== Running Server Tests =====" << std::endl;

    test_utils::run_test("Server Initialization", test_server_init);
    test_utils::run_test("MIME Type Detection", test_mime_types);
    test_utils::run_test("Server Configuration", test_server_config);

    test_utils::print_test_summary();

    return 0;
}