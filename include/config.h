#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct ServerConfig {
    int port = 8080;                         // Default port
    std::string root_directory = "./public"; // Default directory to serve
};

#endif // CONFIG_H