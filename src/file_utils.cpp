#include "../include/file_utils.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>

namespace file_utils {
bool file_exists(const std::string &path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string read_file(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string get_file_extension(const std::string &path) {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        return path.substr(dot_pos);
    }
    return "";
}
} // namespace file_utils