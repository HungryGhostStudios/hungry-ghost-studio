cmake_minimum_required(VERSION 3.24)

add_library(libdsp STATIC
    src/libdsp.cpp
    include/libdsp/util/SmoothParameter.h
    include/libdsp/util/MidSide.h
    include/libdsp/util/Oversampler.h
    include/libdsp/util/LevelMeter.h
    include/libdsp/dynamics/LevelDetector.h
    include/libdsp/dynamics/GainComputer.h
    include/libdsp/filters/SVFFilter.h
    include/libdsp/filters/LadderFilter.h
    include/libdsp/filters/BiquadCascade.h
    include/libdsp/filters/Coefficients.h
    include/libdsp/reverb/DelayLine.h
)

target_include_directories(libdsp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(libdsp PUBLIC cxx_std_17)
