cmake_minimum_required(VERSION 3.13)
project(flutter_tuner LANGUAGES C CXX)

set(CORE_SOURCES tuner.cpp)

add_library(tuner SHARED
        ${CORE_SOURCES}
        jni_bridge.cpp
        tuner_wrapper.cpp)

target_include_directories(tuner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()

find_package(GTest REQUIRED)

add_executable(tuner_test
        tuner_test.cpp
        ${CORE_SOURCES})

target_include_directories(tuner_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(tuner_test GTest::GTest GTest::Main)

add_test(NAME tuner_test COMMAND tuner_test)
