cmake_minimum_required(VERSION 3.21)
project(tfjs_async_to_sync)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXECUTABLE_SUFFIX ".html")
include(FetchContent)

FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.8
)

FetchContent_MakeAvailable(doctest)

file(
        DOWNLOAD
        https://cdn.jsdelivr.net/npm/@tensorflow/tfjs/dist/tf.min.js
        ${CMAKE_CURRENT_BINARY_DIR}/tf.min.js
)

add_executable(tfjs_async_to_sync
        main.cpp
        ${PROJECT_SOURCE_DIR}/src/test_tfjs.cpp
        ${PROJECT_SOURCE_DIR}/src/tfjs.cpp
        ${PROJECT_SOURCE_DIR}/src/sync_to_asnc.cpp
        )
target_include_directories(tfjs_async_to_sync
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${doctest_SOURCE_DIR})

target_compile_options(
        tfjs_async_to_sync
        PRIVATE
        -pthread
)

target_link_options(
        tfjs_async_to_sync
        PRIVATE
        --emrun
        -pthread
        -sPROXY_TO_PTHREAD
        --js-library
        ${PROJECT_SOURCE_DIR}/src/js_functions.js
        # For testing pre-js uncomment this line
#        --pre-js
#        ${CMAKE_CURRENT_BINARY_DIR}/tf.min.js
)
