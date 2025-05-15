#include "../include/config.h"
#include "../include/server.h"
#include <iostream>

int main(int argc, char *argv[]) {
    try {
        // Parse command line arguments to override defaults
        ServerConfig config;

        if (argc > 1) {
            config.port = std::stoi(argv[1]);
        }
        if (argc > 2) {
            config.root_directory = argv[2];
        }

        std::cout << "Starting static file server on port " << config.port
                  << std::endl;
        std::cout << "Serving files from: " << config.root_directory
                  << std::endl;

        // Initialize and start the server
        StaticFileServer server(config);
        server.start();

        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}