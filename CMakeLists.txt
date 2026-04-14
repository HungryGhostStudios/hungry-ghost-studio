cmake_minimum_required(VERSION 3.24)
project(HungryGhost VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/JUCE/CMakeLists.txt)
    add_subdirectory(modules/JUCE)
else()
    message(WARNING "modules/JUCE not found — run 'git submodule update --init --recursive' to fetch JUCE")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/clap-juce-extensions/CMakeLists.txt)
    add_subdirectory(modules/clap-juce-extensions)
else()
    message(WARNING "modules/clap-juce-extensions not found — run 'git submodule update --init --recursive' to fetch clap-juce-extensions")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libdsp/CMakeLists.txt)
    add_subdirectory(libdsp)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/shared/CMakeLists.txt)
    add_subdirectory(plugins/shared)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/hg-compressor/CMakeLists.txt)
    add_subdirectory(plugins/hg-compressor)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/hg-eq/CMakeLists.txt)
    add_subdirectory(plugins/hg-eq)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/hg-saturator/CMakeLists.txt)
    add_subdirectory(plugins/hg-saturator)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/hg-reverb/CMakeLists.txt)
    add_subdirectory(plugins/hg-reverb)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/hg-chorus/CMakeLists.txt)
    add_subdirectory(plugins/hg-chorus)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/hg-synth/CMakeLists.txt)
    add_subdirectory(plugins/hg-synth)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
