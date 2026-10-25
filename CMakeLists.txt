cmake_minimum_required(VERSION 3.22)

project(Kozyavka VERSION 1.2.0)

# Статическая сборка рантайма MSVC: плагину не нужен Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE скачивается автоматически при первой настройке проекта.
# Чтобы использовать уже скачанную копию: -DFETCHCONTENT_SOURCE_DIR_JUCE=путь/к/JUCE
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Kozyavka
    PRODUCT_NAME "Kozyavka.mp3"
    COMPANY_NAME "MyPlugins"
    PLUGIN_MANUFACTURER_CODE Mypl
    PLUGIN_CODE Kzvk
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    FORMATS VST3 Standalone)

target_sources(Kozyavka PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SampleData.cpp
    Source/Presets.cpp
    Source/Visuals.cpp)

target_compile_features(Kozyavka PRIVATE cxx_std_17)

target_compile_definitions(Kozyavka PUBLIC
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Kozyavka
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
