#include "process_executor.h"
#include <android/log.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <array>
#include <sstream>

#define LOG_TAG "RVKProcess"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace rvk {

std::string ProcessExecutor::execute(const std::string& command,
                                     const std::string& workDir,
                                     const std::map<std::string, std::string>& envVars) {
    std::string output;
    executeWithExitCode(command, workDir, envVars, output);
    return output;
}

int ProcessExecutor::executeWithExitCode(const std::string& command,
                                         const std::string& workDir,
                                         const std::map<std::string, std::string>& envVars,
                                         std::string& output) {
    output.clear();
    
    // Build environment string
    std::string envPrefix;
    for (const auto& pair : envVars) {
        envPrefix += "export " + pair.first + "='" + pair.second + "' && ";
    }
    
    // Build full command with cd and env
    std::string fullCommand = envPrefix + "cd '" + workDir + "' && " + command + " 2>&1";
    
    LOGI("Executing: %s", command.c_str());
    LOGI("WorkDir: %s", workDir.c_str());
    
    FILE* pipe = popen(fullCommand.c_str(), "r");
    if (!pipe) {
        output = "ERROR: Failed to execute command\n";
        LOGE("popen failed for: %s", command.c_str());
        return -1;
    }
    
    std::array<char, 4096> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    
    int status = pclose(pipe);
    int exitCode = WEXITSTATUS(status);
    
    LOGI("Command exited with code: %d", exitCode);
    return exitCode;
}

} // namespace rvk
