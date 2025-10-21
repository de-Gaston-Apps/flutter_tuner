cmake_minimum_required(VERSION 3.10)
project(flutter_tuner_native)

# build library
add_library(tuner STATIC
            tuner.cpp
            jni_bridge.cpp
            tuner_wrapper.cpp)

target_include_directories(tuner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# When building for host (e.g. for tests), we need to find JNI.
# When building for Android, the NDK provides JNI headers automatically.
if(NOT ANDROID)
    find_package(JNI REQUIRED)
    if (JNI_FOUND)
        target_include_directories(tuner PUBLIC ${JNI_INCLUDE_DIRS})
    endif()
endif()

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
