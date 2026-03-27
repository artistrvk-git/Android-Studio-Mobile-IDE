#include "file_manager.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <sstream>

namespace rvk {

bool FileManager::fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool FileManager::directoryExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool FileManager::createDirectoryRecursive(const std::string& path) {
    std::string current;
    std::istringstream ss(path);
    std::string token;
    
    while (std::getline(ss, token, '/')) {
        if (token.empty() && current.empty()) {
            current = "/";
            continue;
        }
        if (current.empty() || current == "/") {
            current += token;
        } else {
            current += "/" + token;
        }
        
        struct stat st;
        if (stat(current.c_str(), &st) != 0) {
            if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) {
                return false;
            }
        }
    }
    return true;
}

bool FileManager::deleteRecursive(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return true;
    
    if (S_ISDIR(st.st_mode)) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return false;
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            deleteRecursive(path + "/" + entry->d_name);
        }
        closedir(dir);
        return rmdir(path.c_str()) == 0;
    }
    return unlink(path.c_str()) == 0;
}

long FileManager::getFileSize(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) return st.st_size;
    return -1;
}

std::string FileManager::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool FileManager::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    return file.good();
}

} // namespace rvk
