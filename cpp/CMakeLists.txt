cmake_minimum_required(VERSION 3.10)
project(flutter_tuner_native)

# Define a list of common source files that are always included.
set(TUNER_LIB_SOURCES
    tuner.cpp
    tuner_wrapper.cpp)

# Conditionally add the JNI bridge source file only when building for Android.
if(ANDROID)
    list(APPEND TUNER_LIB_SOURCES jni_bridge.cpp)
endif()

# Build the static library from the source files.
add_library(tuner STATIC ${TUNER_LIB_SOURCES})

target_include_directories(tuner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# GoogleTest (FetchContent) - for running host-side tests
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

# Define the test executable and link it against the tuner library and gtest.
add_executable(tuner_tests tests/test_tuner.cpp)
target_link_libraries(tuner_tests PRIVATE tuner gtest_main)
enable_testing()
add_test(NAME tuner_tests COMMAND tuner_tests)
