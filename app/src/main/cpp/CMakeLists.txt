cmake_minimum_required(VERSION 3.22.1)
project("rvk_engine" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Android libraries
find_library(log-lib log)
find_library(android-lib android)

# Add the native library
add_library(rvk_engine SHARED
    rvk_engine_jni.cpp
    build_engine.cpp
    file_manager.cpp
    process_executor.cpp
)

# Link libraries
target_link_libraries(rvk_engine
    ${log-lib}
    ${android-lib}
)

# Compiler flags
target_compile_options(rvk_engine PRIVATE
    -Wall -Wextra -O2
    -fexceptions -frtti
)
