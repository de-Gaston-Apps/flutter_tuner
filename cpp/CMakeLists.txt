cmake_minimum_required(VERSION 3.4.1)

add_library(tuner SHARED
            tuner.cpp
            jni_bridge.cpp
            tuner_wrapper.cpp)

target_include_directories(tuner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})