#include "build_engine.h"
#include "process_executor.h"
#include "file_manager.h"
#include <android/log.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <sstream>
#include <fstream>

#define LOG_TAG "RVKBuildEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace rvk {

BuildEngine::BuildEngine(const std::string& basePath) : m_basePath(basePath) {
    LOGI("BuildEngine created with base: %s", basePath.c_str());
}

BuildEngine::~BuildEngine() = default;

int BuildEngine::compile(const std::string& projectPath,
                         const std::string& buildType,
                         const std::map<std::string, std::string>& envVars,
                         std::string& output) {
    output.clear();
    output += ">>> RVK Native Build Engine\n";
    output += ">>> Project: " + projectPath + "\n";
    output += ">>> Build Type: " + buildType + "\n";
    
    return buildAndroid(projectPath, buildType, envVars, output);
}

int BuildEngine::buildAndroid(const std::string& projectPath,
                              const std::string& buildType,
                              const std::map<std::string, std::string>& envVars,
                              std::string& output) {
    // Check for gradlew
    std::string gradlew = projectPath + "/gradlew";
    if (!FileManager::fileExists(gradlew)) {
        output += "ERROR: gradlew not found\n";
        return -1;
    }
    
    // CRITICAL: Set executable permissions
    setFilePermissions(gradlew, 0755);
    output += ">>> chmod 755 gradlew\n";
    
    // Find JAVA_HOME
    std::string javaHome = findJavaHome();
    if (javaHome.empty()) {
        output += "ERROR: JAVA_HOME not found\n";
        return -1;
    }
    output += ">>> JAVA_HOME: " + javaHome + "\n";
    
    // Set executable on java binary
    std::string javaBin = javaHome + "/bin/java";
    setFilePermissions(javaBin, 0755);
    
    // Build task
    std::string task = "assembleDebug";
    if (buildType == "release") task = "assembleRelease";
    else if (buildType == "clean") task = "clean";
    
    // Build command using sh (avoids permission issues)
    std::string command = "/system/bin/sh " + gradlew + " " + task +
                          " --stacktrace --no-daemon" +
                          " -Dorg.gradle.java.home=" + javaHome;
    
    output += ">>> Executing: " + command + "\n";
    output += "────────────────────────────────\n";
    
    // Execute via ProcessExecutor
    ProcessExecutor executor;
    std::string cmdOutput = executor.execute(command, projectPath, envVars);
    output += cmdOutput;
    
    // Check for APK
    std::string apkPath = findAPK(projectPath, buildType);
    if (!apkPath.empty()) {
        output += "\n>>> APK generated: " + apkPath + "\n";
        return 0;
    }
    
    // Check build output for success indication
    if (cmdOutput.find("BUILD SUCCESSFUL") != std::string::npos) {
        return 0;
    }
    
    return -1;
}

std::string BuildEngine::findJavaHome() const {
    std::vector<std::string> paths = {
        m_basePath + "/JDK/jdk-17",
        m_basePath + "/JDK/jdk-21",
        m_basePath + "/JDK/jdk17",
        m_basePath + "/JDK/jdk21",
        m_basePath + "/JDK/openjdk-17"
    };
    
    for (const auto& path : paths) {
        std::string javaBin = path + "/bin/java";
        if (FileManager::fileExists(javaBin)) {
            return path;
        }
    }
    
    // Deep search
    std::string jdkDir = m_basePath + "/JDK";
    DIR* dir = opendir(jdkDir.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
                std::string candidate = jdkDir + "/" + entry->d_name + "/bin/java";
                if (FileManager::fileExists(candidate)) {
                    closedir(dir);
                    return jdkDir + "/" + entry->d_name;
                }
            }
        }
        closedir(dir);
    }
    
    return "";
}

std::string BuildEngine::findGradleHome() const {
    std::vector<std::string> versions = {"8.14.4", "9.4.0", "9.3.0"};
    for (const auto& ver : versions) {
        std::string path = m_basePath + "/Gradle/gradle-" + ver;
        if (FileManager::fileExists(path + "/bin/gradle")) {
            return path;
        }
    }
    return "";
}

void BuildEngine::setFilePermissions(const std::string& path, mode_t mode) {
    chmod(path.c_str(), mode);
}

void BuildEngine::setExecutableRecursive(const std::string& dirPath) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        
        std::string fullPath = dirPath + "/" + entry->d_name;
        
        if (entry->d_type == DT_DIR) {
            setExecutableRecursive(fullPath);
        } else if (entry->d_type == DT_REG) {
            // Set executable on bin files
            std::string name(entry->d_name);
            if (name.find('.') == std::string::npos || // no extension
                name.substr(name.size() - 3) == ".sh") { // shell scripts
                chmod(fullPath.c_str(), 0755);
            }
        }
    }
    closedir(dir);
}

std::string BuildEngine::findAPK(const std::string& projectPath, const std::string& buildType) {
    std::vector<std::string> searchPaths = {
        projectPath + "/app/build/outputs/apk/debug",
        projectPath + "/app/build/outputs/apk/release",
        projectPath + "/app/build/outputs/apk",
        projectPath + "/build/outputs/apk/debug",
        projectPath + "/build/outputs/apk"
    };
    
    for (const auto& searchPath : searchPaths) {
        DIR* dir = opendir(searchPath.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string name(entry->d_name);
                if (name.size() > 4 && name.substr(name.size() - 4) == ".apk") {
                    closedir(dir);
                    return searchPath + "/" + name;
                }
            }
            closedir(dir);
        }
    }
    
    return "";
}

} // namespace rvk
