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

# Test executable (without jni_bridge.cpp to avoid JNI dependency)
add_executable(tuner_test
        tuner_test.cpp
        tuner.cpp
        yinfft.c
)

target_include_directories(tuner_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(UNIX AND NOT APPLE)
    target_link_libraries(tuner_test PRIVATE m)
endif()
