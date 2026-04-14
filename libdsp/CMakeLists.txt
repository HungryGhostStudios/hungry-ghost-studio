cmake_minimum_required(VERSION 3.24)

add_library(libdsp STATIC
    src/libdsp.cpp
)

target_include_directories(libdsp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(libdsp PUBLIC cxx_std_17)
