cmake_minimum_required(VERSION 3.4.1)
project(tuner)

find_package(JNI REQUIRED)

add_library(tuner SHARED
            tuner.cpp
            jni_bridge.cpp
            tuner_wrapper.cpp)

target_include_directories(tuner PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${JNI_INCLUDE_DIRS}
)

target_link_libraries(tuner PRIVATE ${JNI_LIBRARIES})

if(ANDROID)
    target_link_options(tuner PRIVATE "-Wl,-z,max-page-size=16384")
endif()