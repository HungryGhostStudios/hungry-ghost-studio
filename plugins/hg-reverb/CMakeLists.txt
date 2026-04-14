cmake_minimum_required(VERSION 3.24)

juce_add_plugin(hg-reverb
    PLUGIN_MANUFACTURER_CODE HGPl
    PLUGIN_CODE HGRv
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "HG Reverb"
    COMPANY_NAME "Hungry Ghost Studios"
)

target_sources(hg-reverb PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
)

target_include_directories(hg-reverb PRIVATE
    src
)

target_link_libraries(hg-reverb PRIVATE
    libdsp
    hg-shared
    juce::juce_audio_utils
    juce::juce_dsp
)

target_compile_features(hg-reverb PRIVATE cxx_std_17)

target_compile_definitions(hg-reverb PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

if(TARGET clap_juce_extensions)
    clap_juce_extensions_plugin(TARGET hg-reverb
        CLAP_ID "com.hungryghoststu.hg-reverb"
        CLAP_FEATURES "audio-effect" "reverb"
    )
endif()
