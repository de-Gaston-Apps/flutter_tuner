cmake_minimum_required(VERSION 3.13)
project(flutter_tuner LANGUAGES C CXX)

find_package(JNI REQUIRED)

add_library(tuner SHARED
        tuner.cpp
        jni_bridge.cpp
        tuner_wrapper.cpp)

target_include_directories(tuner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${JNI_INCLUDE_DIRS})
target_link_libraries(tuner PRIVATE ${JNI_LIBRARIES})

# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    tuner_test
    tuner_test.cpp
)

target_link_libraries(
    tuner_test
    tuner
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(tuner_test)
