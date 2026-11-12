cmake_minimum_required(VERSION 3.22)

project(SHEETZ VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SHEETZ
    COMPANY_NAME               "Sheetz Audio"
    BUNDLE_ID                  "com.sheetzaudio.sheetz"
    DESCRIPTION                "SHEETZ! - cloud modulation shaper"
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD    FALSE
    PLUGIN_MANUFACTURER_CODE   SHET
    PLUGIN_CODE                SHZ1
    FORMATS                    VST3 Standalone
    VST3_CATEGORIES            Fx
    PRODUCT_NAME               "SHEETZ!"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT    "SHEETZ! processes audio input from your audio interface."
)

target_sources(SHEETZ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Curve.cpp
    Source/FactoryPresets.cpp
    Source/CloudsLookAndFeel.cpp
    Source/CurveEditor.cpp
    Source/ModuleButton.cpp
)

target_include_directories(SHEETZ PRIVATE Source)

target_compile_definitions(SHEETZ PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

target_link_libraries(SHEETZ
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

juce_generate_juce_header(SHEETZ)
