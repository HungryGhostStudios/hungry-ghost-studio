cmake_minimum_required(VERSION 3.24)

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(libdsp_tests
    libdsp/test_smooth_parameter.cpp
    libdsp/test_midside.cpp
    libdsp/test_ladder_filter.cpp
    libdsp/test_biquad_cascade.cpp
    libdsp/test_waveshaper.cpp
)

target_link_libraries(libdsp_tests
    PRIVATE
        libdsp
        Catch2::Catch2WithMain
)

target_compile_features(libdsp_tests PRIVATE cxx_std_17)

include(CTest)
include(Catch)
catch_discover_tests(libdsp_tests)
