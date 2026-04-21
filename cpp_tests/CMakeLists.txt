cmake_minimum_required(VERSION 3.13)
project(tuner_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to the shared tuner source code
set(TUNER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ios/Classes/cpp")

# Test executable — references the production source but lives outside the package
add_executable(tuner_test
    tuner_test.cpp
    ${TUNER_SRC_DIR}/tuner.cpp
    ${TUNER_SRC_DIR}/yinfft.c
)

target_include_directories(tuner_test PRIVATE
    ${TUNER_SRC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(tuner_test PRIVATE m)
endif()

# Copy test fixture WAV files to the build directory so the test binary can find them
file(GLOB WAV_FIXTURES "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/*.wav")
foreach(WAV_FILE ${WAV_FIXTURES})
    get_filename_component(WAV_NAME ${WAV_FILE} NAME)
    configure_file(${WAV_FILE} ${CMAKE_CURRENT_BINARY_DIR}/fixtures/${WAV_NAME} COPYONLY)
endforeach()
