# ==============================================================================
# mettavoxad — Processador Vocal v2.1.0 (Windows x86/x64, legado + VST3)
# Plataformas Alvo: Windows (x86_64 MSVC/Clang) e Linux (x86_64 GCC/Clang)
# Hosts Alvo: MAGIX Sound Forge, MAGIX Samplitude e Cockos REAPER
# ==============================================================================
cmake_minimum_required(VERSION 3.22)
project(mettavoxad VERSION 2.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Static runtime removes dependency on an installed Visual C++ redistributable.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(mettavoxad
    COMPANY_NAME                "mettavoxad Audio Engineering"
    PLUGIN_MANUFACTURER_CODE    Mtvx
    PLUGIN_CODE                 Mt21
    FORMATS                     VST3
    PRODUCT_NAME                "METTAVOXAD21"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             "Fx" "Vocal" "Dynamics" "EQ" "Reverb"
)

target_compile_options(mettavoxad PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

target_sources(mettavoxad
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/DspModules.h
        Source/VoiceAssistantAnalyzer.h
        Source/AdvancedEngines.h
)

target_compile_definitions(mettavoxad
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(mettavoxad
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# Original legacy ABI adapter: same JUCE processor/editor, no external VST2 SDK.
option(METTAVOXAD_BUILD_LEGACY "Build legacy VST DLL for Sound Forge 8 and other VST hosts" ON)
if(METTAVOXAD_BUILD_LEGACY)
    add_library(mettavoxad_Legacy MODULE Source/LegacyVstWrapper.cpp)
    target_link_libraries(mettavoxad_Legacy PRIVATE mettavoxad)
    target_include_directories(mettavoxad_Legacy PRIVATE Source
        $<TARGET_PROPERTY:mettavoxad,INCLUDE_DIRECTORIES>)
    target_compile_definitions(mettavoxad_Legacy PRIVATE
        $<TARGET_PROPERTY:mettavoxad,COMPILE_DEFINITIONS>)
    set_target_properties(mettavoxad_Legacy PROPERTIES
        PREFIX "" OUTPUT_NAME "METTAVOXAD21"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mettavoxad_artefacts/$<CONFIG>/VST"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mettavoxad_artefacts/$<CONFIG>/VST")
    if(MSVC)
        # LINK resolves C symbol decoration for x86; keep aliases undecorated.
        target_link_options(mettavoxad_Legacy PRIVATE
            "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/Source/LegacyVst.def")
    endif()
endif()
if(METTAVOXAD_BUILD_LEGACY)
    add_executable(legacy_smoke EXCLUDE_FROM_ALL Tests/LegacySmoke.cpp)
    target_include_directories(legacy_smoke PRIVATE Source)
    target_link_libraries(legacy_smoke PRIVATE ${CMAKE_DL_LIBS})
endif()
add_executable(dsp_checks EXCLUDE_FROM_ALL Tests/DspChecks.cpp)
target_link_libraries(dsp_checks PRIVATE mettavoxad)
target_include_directories(dsp_checks PRIVATE Source $<TARGET_PROPERTY:mettavoxad,INCLUDE_DIRECTORIES>)
target_compile_definitions(dsp_checks PRIVATE $<TARGET_PROPERTY:mettavoxad,COMPILE_DEFINITIONS>)
