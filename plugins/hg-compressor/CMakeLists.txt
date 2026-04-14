juce_add_plugin(hg-compressor
    COMPANY_NAME "Hungry Ghost"
    PLUGIN_MANUFACTURER_CODE HGPl
    PLUGIN_CODE HGCp
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "HG Compressor"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_SYNTH FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(hg-compressor PRIVATE
    src/PluginProcessor.cpp
)

target_include_directories(hg-compressor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(hg-compressor
    PRIVATE
        libdsp
        hg-shared
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(hg-compressor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_compile_features(hg-compressor PRIVATE cxx_std_17)

if(TARGET clap_juce_extensions)
    clap_juce_extensions_plugin(TARGET hg-compressor
        CLAP_ID "com.hungryghost.compressor"
        CLAP_FEATURES "audio-effect" "compressor" "dynamics"
    )
endif()
