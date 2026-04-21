cmake_minimum_required(VERSION 3.13)
project(flutter_tuner LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ANDROID)
    add_library(tuner SHARED
            tuner.cpp
            jni_bridge.cpp
            yinfft.c
    )
else()
    add_library(tuner SHARED
            tuner.cpp
            yinfft.c
    )
endif()

target_include_directories(tuner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Tests have been moved to cpp_tests/ at the project root.
# See cpp_tests/CMakeLists.txt for the test build configuration.
