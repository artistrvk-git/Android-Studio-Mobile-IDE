/**
 * RVK Engine JNI Bridge
 * Connects Kotlin/Java frontend with C++ backend
 * Android Studio Mobile - Presented By RVK EDITION
 */

#include <jni.h>
#include <string>
#include <android/log.h>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cerrno>
#include <vector>
#include <map>
#include <cstring>

#include "build_engine.h"
#include "process_executor.h"
#include "file_manager.h"

#define LOG_TAG "RVKEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global state
static std::string g_base_path;
static std::string g_last_build_output;
static std::map<std::string, std::string> g_env_vars;
static rvk::BuildEngine* g_build_engine = nullptr;
static rvk::ProcessExecutor* g_process_executor = nullptr;

extern "C" {

/**
 * Initialize the RVK Engine
 */
JNIEXPORT jboolean JNICALL
Java_com_rvk_androidstudiomobile_core_engine_RVKEngine_nativeInit(
    JNIEnv *env, jclass clazz, jstring base_path) {
    
    const char* path = env->GetStringUTFChars(base_path, nullptr);
    g_base_path = std::string(path);
    env->ReleaseStringUTFChars(base_path, path);
    
    LOGI("RVK Engine initializing at: %s", g_base_path.c_str());
    
    // Create required directories
    std::vector<std::string> dirs = {
        g_base_path + "/Projects",
        g_base_path + "/SDK",
        g_base_path + "/JDK",
        g_base_path + "/NDK",
        g_base_path + "/Gradle",
        g_base_path + "/Flutter",
        g_base_path + "/NodeJS",
        g_base_path + "/Python",
        g_base_path + "/.temp",
        g_base_path + "/.cache",
        g_base_path + "/Logs"
    };
    
    for (const auto& dir : dirs) {
        rvk::FileManager::createDirectoryRecursive(dir);
    }
    
    // Initialize build engine
    g_build_engine = new rvk::BuildEngine(g_base_path);
    g_process_executor = new rvk::ProcessExecutor();
    
    // Set default environment variables
    g_env_vars["HOME"] = g_base_path;
    g_env_vars["TMPDIR"] = g_base_path + "/.temp";
    g_env_vars["ANDROID_HOME"] = g_base_path + "/SDK";
    g_env_vars["ANDROID_SDK_ROOT"] = g_base_path + "/SDK";
    
    // Auto-detect JAVA_HOME
    std::vector<std::string> jdk_paths = {
        g_base_path + "/JDK/jdk-17",
        g_base_path + "/JDK/jdk-21",
        g_base_path + "/JDK/jdk17",
        g_base_path + "/JDK/jdk21"
    };
    
    for (const auto& jdk : jdk_paths) {
        std::string java_bin = jdk + "/bin/java";
        if (rvk::FileManager::fileExists(java_bin)) {
            g_env_vars["JAVA_HOME"] = jdk;
            // Set executable permission
            chmod(java_bin.c_str(), 0755);
            std::string javac_bin = jdk + "/bin/javac";
            if (rvk::FileManager::fileExists(javac_bin)) {
                chmod(javac_bin.c_str(), 0755);
            }
            LOGI("JAVA_HOME set to: %s", jdk.c_str());
            break;
        }
    }
    
    LOGI("RVK Engine initialized successfully");
    return JNI_TRUE;
}

/**
 * Compile a project
 */
JNIEXPORT jint JNICALL
Java_com_rvk_androidstudiomobile_core_engine_RVKEngine_nativeCompile(
    JNIEnv *env, jclass clazz, jstring project_path, jstring build_type) {
    
    const char* path = env->GetStringUTFChars(project_path, nullptr);
    const char* type = env->GetStringUTFChars(build_type, nullptr);
    
    std::string projPath(path);
    std::string buildType(type);
    
    env->ReleaseStringUTFChars(project_path, path);
    env->ReleaseStringUTFChars(build_type, type);
    
    LOGI("Compiling: %s (type: %s)", projPath.c_str(), buildType.c_str());
    
    if (g_build_engine == nullptr) {
        g_last_build_output = "ERROR: Build engine not initialized";
        return -1;
    }
    
    // Set permissions on gradlew
    std::string gradlew = projPath + "/gradlew";
    if (rvk::FileManager::fileExists(gradlew)) {
        chmod(gradlew.c_str(), 0755);
        LOGI("Set execute permission on gradlew");
    }
    
    int result = g_build_engine->compile(projPath, buildType, g_env_vars, g_last_build_output);
    return result;
}

/**
 * Get build output
 */
JNIEXPORT jstring JNICALL
Java_com_rvk_androidstudiomobile_core_engine_RVKEngine_nativeGetBuildOutput(
    JNIEnv *env, jclass clazz) {
    return env->NewStringUTF(g_last_build_output.c_str());
}

/**
 * Set environment variable
 */
JNIEXPORT jboolean JNICALL
Java_com_rvk_androidstudiomobile_core_engine_RVKEngine_nativeSetEnvVar(
    JNIEnv *env, jclass clazz, jstring key, jstring value) {
    
    const char* k = env->GetStringUTFChars(key, nullptr);
    const char* v = env->GetStringUTFChars(value, nullptr);
    
    g_env_vars[std::string(k)] = std::string(v);
    
    env->ReleaseStringUTFChars(key, k);
    env->ReleaseStringUTFChars(value, v);
    
    return JNI_TRUE;
}

/**
 * Execute a shell command
 */
JNIEXPORT jstring JNICALL
Java_com_rvk_androidstudiomobile_core_engine_RVKEngine_nativeExecuteCommand(
    JNIEnv *env, jclass clazz, jstring command, jstring work_dir) {
    
    const char* cmd = env->GetStringUTFChars(command, nullptr);
    const char* dir = env->GetStringUTFChars(work_dir, nullptr);
    
    std::string result;
    if (g_process_executor) {
        result = g_process_executor->execute(std::string(cmd), std::string(dir), g_env_vars);
    } else {
        result = "ERROR: Process executor not initialized";
    }
    
    env->ReleaseStringUTFChars(command, cmd);
    env->ReleaseStringUTFChars(work_dir, dir);
    
    return env->NewStringUTF(result.c_str());
}

/**
 * Get engine version
 */
JNIEXPORT jstring JNICALL
Java_com_rvk_androidstudiomobile_core_engine_RVKEngine_nativeGetVersion(
    JNIEnv *env, jclass clazz) {
    return env->NewStringUTF("RVK Engine v1.0.0 - Presented By RVK EDITION");
}

} // extern "C"
