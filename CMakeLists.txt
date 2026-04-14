cmake_minimum_required(VERSION 3.24)
project(HungryGhost VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(modules/JUCE)
add_subdirectory(modules/clap-juce-extensions)

add_subdirectory(libdsp)

add_subdirectory(plugins/shared)
add_subdirectory(plugins/hg-compressor)
add_subdirectory(plugins/hg-eq)
add_subdirectory(plugins/hg-saturator)
add_subdirectory(plugins/hg-reverb)
add_subdirectory(plugins/hg-chorus)
add_subdirectory(plugins/hg-synth)

add_subdirectory(tests)
