#pragma once

#include <string>
#include <map>
#include <vector>

namespace rvk {

class BuildEngine {
public:
    explicit BuildEngine(const std::string& basePath);
    ~BuildEngine();
    
    int compile(const std::string& projectPath, 
                const std::string& buildType,
                const std::map<std::string, std::string>& envVars,
                std::string& output);
    
    std::string findJavaHome() const;
    std::string findGradleHome() const;
    
private:
    std::string m_basePath;
    
    int buildAndroid(const std::string& projectPath,
                     const std::string& buildType,
                     const std::map<std::string, std::string>& envVars,
                     std::string& output);
    
    void setFilePermissions(const std::string& path, mode_t mode);
    void setExecutableRecursive(const std::string& dirPath);
    std::string findAPK(const std::string& projectPath, const std::string& buildType);
};

} // namespace rvk
