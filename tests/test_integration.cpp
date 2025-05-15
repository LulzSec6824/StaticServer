#include "../include/config.h"
#include "../include/file_utils.h"
#include "../include/server.h"
#include "test_utils.hpp"
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

// Constants for testing
const int TEST_PORT = 8082;
const std::string TEST_DIR = "./test_public";
const std::string TEST_FILE = "test_index.html";
const std::string TEST_CONTENT = "<html><body>Test Content</body></html>";
const int SOCKET_TIMEOUT_SECONDS = 3;

// Test fixture for integration tests
class ServerIntegrationTest {
  public:
    ServerIntegrationTest() : should_stop(false) {
        // Setup test environment
        test_utils::ensure_directory(TEST_DIR);
        test_utils::create_test_file(TEST_DIR + "/" + TEST_FILE, TEST_CONTENT);

        // Configure server
        config.port = TEST_PORT;
        config.root_directory = TEST_DIR;
    }

    ~ServerIntegrationTest() {
        // Ensure server is stopped
        stop_server();

        // Cleanup test environment
        test_utils::cleanup_test_file(TEST_DIR + "/" + TEST_FILE);
        if (system(("rmdir " + TEST_DIR).c_str()) != 0) {
            std::cerr << "Warning: Failed to remove directory: " << TEST_DIR
                      << std::endl;
        }
    }

    // Start server in a separate thread with proper termination support
    std::thread start_server() {
        should_stop = false;
        return std::thread([this]() {
            try {
                StaticFileServer server(config);
                server_started = true;

                // In a real implementation, you would modify the server class
                // to support graceful shutdown by checking the should_stop flag
                while (!should_stop) {
                    server.start();
                    // Add a small delay to prevent CPU spinning
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            } catch (const std::exception &e) {
                std::cerr << "Server error: " << e.what() << std::endl;
            }
        });
    }

    // Stop the server gracefully
    void stop_server() {
        should_stop = true;

        // Give the server thread time to clean up
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Set socket timeout
    bool set_socket_timeout(int sock, int seconds) {
        struct timeval timeout;
        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;

        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                       sizeof(timeout)) < 0) {
            return false;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                       sizeof(timeout)) < 0) {
            return false;
        }

        return true;
    }

    // Simulate making an HTTP request with timeout handling
    std::string make_request(const std::string &path) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return "ERROR: Could not create socket";
        }

        // Set socket timeout
        if (!set_socket_timeout(sock, SOCKET_TIMEOUT_SECONDS)) {
            close(sock);
            return "ERROR: Could not set socket timeout";
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(TEST_PORT);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&server_addr,
                    sizeof(server_addr)) < 0) {
            close(sock);
            return "ERROR: Connection failed";
        }

        std::string request =
            "GET " + path + " HTTP/1.1\r\nHost: localhost\r\n\r\n";
        if (send(sock, request.c_str(), request.length(), 0) < 0) {
            close(sock);
            return "ERROR: Failed to send request";
        }

        char buffer[4096] = {0};
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        close(sock);

        if (bytes_received <= 0) {
            return "ERROR: No response received";
        }

        return std::string(buffer, bytes_received);
    }

    // Parse HTTP response to extract status code
    int get_response_status(const std::string &response) {
        size_t pos = response.find("HTTP/1.1 ");
        if (pos == std::string::npos) {
            return -1;
        }

        pos += 9; // Skip "HTTP/1.1 "
        return std::stoi(response.substr(pos, 3));
    }

    ServerConfig config;
    std::atomic<bool> should_stop;
    std::atomic<bool> server_started{false};
};

// Test for overall integration of components
void test_component_integration() {
    // Test that file utilities can access files in the directory that server
    // would serve
    ServerConfig config;
    config.root_directory = "./public";

    // Ensure the directory exists and has an index.html
    test_utils::ensure_directory(config.root_directory);
    test_utils::create_test_file(config.root_directory +
                                     "/test_integration.html",
                                 "<html><body>Integration Test</body></html>");

    // Test file detection
    bool file_exists = file_utils::file_exists(config.root_directory +
                                               "/test_integration.html");
    test_utils::test_assert(
        file_exists,
        "File utilities should detect file in server root directory");

    // Test file reading
    std::string content =
        file_utils::read_file(config.root_directory + "/test_integration.html");
    test_utils::test_assert(
        content == "<html><body>Integration Test</body></html>",
        "File utilities should read file content correctly");

    // Clean up
    test_utils::cleanup_test_file(config.root_directory +
                                  "/test_integration.html");
}

// Mock socket test
void test_socket_creation() {
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    test_utils::test_assert(sock >= 0, "Should be able to create a socket");

    // Basic socket configuration
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0); // Let OS choose a port
    addr.sin_addr.s_addr = INADDR_ANY;

    // Test binding
    int bind_result = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    test_utils::test_assert(bind_result >= 0,
                            "Should be able to bind to a port");

    // Test listening
    int listen_result = listen(sock, 1);
    test_utils::test_assert(listen_result >= 0,
                            "Should be able to listen on socket");

    // Clean up
    close(sock);
}

// Full server integration test
void test_full_server_integration() {
    ServerIntegrationTest test_fixture;

    // Start server in background
    std::thread server_thread = test_fixture.start_server();

    // Allow server to initialize
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Test existing file
    std::string response = test_fixture.make_request("/" + TEST_FILE);
    test_utils::test_assert(response.find("HTTP/1.1 200 OK") !=
                                std::string::npos,
                            "Server should return 200 OK for existing file");
    test_utils::test_assert(response.find(TEST_CONTENT) != std::string::npos,
                            "Response should contain file content");

    // Test non-existent file
    std::string not_found = test_fixture.make_request("/not_found.html");
    test_utils::test_assert(not_found.find("HTTP/1.1 404 Not Found") !=
                                std::string::npos,
                            "Server should return 404 for non-existent file");

    // Clean up - stop server and join thread
    test_fixture.stop_server();
    if (server_thread.joinable()) {
        server_thread.join();
    }
}

int main() {
    std::cout << "===== Running Integration Tests =====" << std::endl;

    test_utils::run_test("Component Integration", test_component_integration);
    test_utils::run_test("Socket Creation", test_socket_creation);
    test_utils::run_test("Full Server Integration",
                         test_full_server_integration);

    test_utils::print_test_summary();

    return 0;
}
