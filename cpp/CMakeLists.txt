cmake_minimum_required(VERSION 3.10)
project(flutter_tuner_native)

# Find JNI
find_package(JNI REQUIRED)

# build library
add_library(tuner STATIC
            tuner.cpp
            jni_bridge.cpp
            tuner_wrapper.cpp)

target_include_directories(tuner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${JNI_INCLUDE_DIRS})

# GoogleTest (FetchContent)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

# tests
add_executable(tuner_tests tests/test_tuner.cpp)
target_link_libraries(tuner_tests PRIVATE tuner gtest_main)
enable_testing()
add_test(NAME tuner_tests COMMAND tuner_tests)
