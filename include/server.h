#ifndef STATIC_FILE_SERVER_H
#define STATIC_FILE_SERVER_H

#include "config.h"
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unordered_map>

class StaticFileServer {
  public:
    StaticFileServer(const ServerConfig &config);
    ~StaticFileServer();

    void start();

  private:
    int server_fd;
    ServerConfig config;
    std::unordered_map<std::string, std::string> mime_types;

    void initialize_socket();
    void handle_connection(int client_socket);
    void parse_request(const std::string &request, std::string &path,
                       std::string &method);
    void send_response(int client_socket, const std::string &path);
    std::string get_content_type(const std::string &path);
    void initialize_mime_types();
};

#endif // STATIC_FILE_SERVER_H