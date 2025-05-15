#include "../include/server.h"
#include "../include/file_utils.h"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

StaticFileServer::StaticFileServer(const ServerConfig &config)
    : config(config) {
    initialize_mime_types();
    initialize_socket();
}

StaticFileServer::~StaticFileServer() {
    if (server_fd >= 0) {
        close(server_fd);
    }
}

void StaticFileServer::initialize_mime_types() {
    mime_types = {
        {".html", "text/html"},        {".htm", "text/html"},
        {".css", "text/css"},          {".js", "application/javascript"},
        {".json", "application/json"}, {".png", "image/png"},
        {".jpg", "image/jpeg"},        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},         {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},      {".txt", "text/plain"},
        {".pdf", "application/pdf"}};
}

void StaticFileServer::initialize_socket() {
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0) {
        close(server_fd);
        throw std::runtime_error("Failed to set socket options");
    }

    // Bind socket to port
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(config.port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to bind socket to port");
    }

    // Start listening
    if (listen(server_fd, 10) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }
}

void StaticFileServer::start() {
    std::cout << "Server started. Press Ctrl+C to stop.\n" << std::endl;

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (true) {
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr,
                                   &client_addr_len);
        if (client_socket < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }

        // Get client IP
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "Connection from " << client_ip << std::endl;

        // Handle the connection
        handle_connection(client_socket);
        close(client_socket);
    }
}

void StaticFileServer::handle_connection(int client_socket) {
    // Read request
    char buffer[1024] = {0};
    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);

    if (bytes_read <= 0) {
        std::cerr << "Failed to read from client" << std::endl;
        return;
    }

    std::string request(buffer);

    // Parse request to get the path
    std::string path, method;
    parse_request(request, path, method);

    // Only handle GET requests
    if (method != "GET") {
        std::string response = "HTTP/1.1 405 Method Not Allowed\r\n"
                               "Content-Length: 0\r\n"
                               "\r\n";
        send(client_socket, response.c_str(), response.length(), 0);
        return;
    }

    // Send response
    send_response(client_socket, path);
}

void StaticFileServer::parse_request(const std::string &request,
                                     std::string &path, std::string &method) {
    std::istringstream request_stream(request);

    // Get the first line
    std::string first_line;
    std::getline(request_stream, first_line, '\r');

    // Parse the first line
    std::istringstream first_line_stream(first_line);
    first_line_stream >> method >> path;

    // Ensure path starts from root directory
    if (path == "/") {
        path = "/index.html"; // Default to index.html
    }

    // Remove URL parameters if any
    size_t param_pos = path.find('?');
    if (param_pos != std::string::npos) {
        path = path.substr(0, param_pos);
    }
}

void StaticFileServer::send_response(int client_socket,
                                     const std::string &path) {
    // Form the full file path
    std::string full_path = config.root_directory + path;

    // Check if the file exists and is readable
    if (!file_utils::file_exists(full_path)) {
        std::string not_found = "HTTP/1.1 404 Not Found\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: 9\r\n"
                                "\r\n"
                                "Not Found";
        send(client_socket, not_found.c_str(), not_found.length(), 0);
        return;
    }

    // Get the file content
    std::string content;
    try {
        content = file_utils::read_file(full_path);
    } catch (const std::exception &e) {
        std::string server_error = "HTTP/1.1 500 Internal Server Error\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "Content-Length: 21\r\n"
                                   "\r\n"
                                   "Internal Server Error";
        send(client_socket, server_error.c_str(), server_error.length(), 0);
        return;
    }

    // Send the file with appropriate headers
    std::string content_type = get_content_type(path);
    std::string response_header = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: " +
                                  content_type +
                                  "\r\n"
                                  "Content-Length: " +
                                  std::to_string(content.length()) +
                                  "\r\n"
                                  "\r\n";

    // Send headers
    send(client_socket, response_header.c_str(), response_header.length(), 0);

    // Send file content
    send(client_socket, content.c_str(), content.length(), 0);
}

std::string StaticFileServer::get_content_type(const std::string &path) {
    // Extract file extension
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string ext = path.substr(dot_pos);

        // Look up MIME type
        auto it = mime_types.find(ext);
        if (it != mime_types.end()) {
            return it->second;
        }
    }

    // Default to binary
    return "application/octet-stream";
}