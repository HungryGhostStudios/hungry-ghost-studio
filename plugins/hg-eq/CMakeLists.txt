cmake_minimum_required(VERSION 3.24)

juce_add_plugin(hg-eq
    PLUGIN_MANUFACTURER_CODE HGPl
    PLUGIN_CODE HGEq
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "HG EQ"
    COMPANY_NAME "Hungry Ghost Studios"
)

target_sources(hg-eq PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
)

target_include_directories(hg-eq PRIVATE
    src
)

target_link_libraries(hg-eq PRIVATE
    libdsp
    hg-shared
    juce::juce_audio_utils
    juce::juce_dsp
)

target_compile_features(hg-eq PRIVATE cxx_std_17)

target_compile_definitions(hg-eq PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

if(TARGET clap_juce_extensions)
    clap_juce_extensions_plugin(TARGET hg-eq
        CLAP_ID "com.hungryghoststu.hg-eq"
        CLAP_FEATURES "audio-effect" "equalizer"
    )
endif()
