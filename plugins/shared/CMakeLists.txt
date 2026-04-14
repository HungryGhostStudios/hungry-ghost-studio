add_library(hg-shared STATIC
    src/HGLookAndFeel.cpp
)

target_include_directories(hg-shared PUBLIC src)

target_link_libraries(hg-shared PUBLIC juce::juce_gui_basics)

target_compile_features(hg-shared PUBLIC cxx_std_17)
