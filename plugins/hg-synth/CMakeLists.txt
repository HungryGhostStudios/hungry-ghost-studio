juce_add_plugin(hg-synth
    COMPANY_NAME "Hungry Ghost"
    PLUGIN_MANUFACTURER_CODE HGPl
    PLUGIN_CODE HGSy
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "HG Synth"
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_SYNTH TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(hg-synth PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
)

target_include_directories(hg-synth PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(hg-synth
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

target_compile_definitions(hg-synth
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_compile_features(hg-synth PRIVATE cxx_std_17)

if(TARGET clap_juce_extensions)
    clap_juce_extensions_plugin(TARGET hg-synth
        CLAP_ID "com.hungryghost.synth"
        CLAP_FEATURES "instrument" "synthesizer"
    )
endif()
