#pragma once

#include <string>

namespace rvk {

class FileManager {
public:
    static bool fileExists(const std::string& path);
    static bool directoryExists(const std::string& path);
    static bool createDirectoryRecursive(const std::string& path);
    static bool deleteRecursive(const std::string& path);
    static long getFileSize(const std::string& path);
    static std::string readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::string& content);
};

} // namespace rvk
