#pragma once

#include <string>
#include <map>

namespace rvk {

class ProcessExecutor {
public:
    ProcessExecutor() = default;
    ~ProcessExecutor() = default;
    
    std::string execute(const std::string& command,
                       const std::string& workDir,
                       const std::map<std::string, std::string>& envVars);
    
    int executeWithExitCode(const std::string& command,
                           const std::string& workDir,
                           const std::map<std::string, std::string>& envVars,
                           std::string& output);
};

} // namespace rvk
