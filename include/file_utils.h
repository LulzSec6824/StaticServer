#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>

namespace file_utils {
bool file_exists(const std::string &path);
std::string read_file(const std::string &path);
std::string get_file_extension(const std::string &path);
} // namespace file_utils

#endif // FILE_UTILS_H