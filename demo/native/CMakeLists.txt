cmake_minimum_required(VERSION 3.20)
project(matching_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(JNI REQUIRED)

set(NATIVE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${NATIVE_OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${NATIVE_OUTPUT_DIR}")

foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${CONFIG}" CONFIG_UPPER)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${NATIVE_OUTPUT_DIR}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${NATIVE_OUTPUT_DIR}")
endforeach()

add_library(matching_engine SHARED
    src/matching_engine.cpp
    src/jni_bridge.cpp
)

target_include_directories(matching_engine PRIVATE
    include
    ${JNI_INCLUDE_DIRS}
)

target_link_libraries(matching_engine PRIVATE ${JNI_LIBRARIES})

if(MSVC)
    target_compile_options(matching_engine PRIVATE /W4 /permissive-)
else()
    target_compile_options(matching_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
