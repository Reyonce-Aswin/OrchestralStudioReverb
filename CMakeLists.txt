cmake_minimum_required(VERSION 3.22)
project(OrchestralStudioReverb VERSION 0.1.0 LANGUAGES C CXX)

option(OSR_BUILD_TESTS "Build standalone DSP tests" ON)
option(OSR_BUILD_VST3 "Build the JUCE VST3 plugin when JUCE is available" ON)
option(OSR_FETCH_JUCE "Fetch JUCE from GitHub if it is not already available" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(osr_dsp STATIC
    Source/DSP/FDNReverb.cpp
    Source/DSP/EarlyReflections.cpp
)
target_include_directories(osr_dsp PUBLIC Source/DSP)
target_compile_features(osr_dsp PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(osr_dsp PRIVATE /W4 /permissive-)
else()
    target_compile_options(osr_dsp PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(OSR_BUILD_TESTS)
    enable_testing()

    add_executable(osr_fdn_tests Tests/FDNTests.cpp)
    target_link_libraries(osr_fdn_tests PRIVATE osr_dsp)
    add_test(NAME osr_fdn_tests COMMAND osr_fdn_tests)

    add_executable(osr_early_reflections_tests Tests/EarlyReflectionsTests.cpp)
    target_link_libraries(osr_early_reflections_tests PRIVATE osr_dsp)
    add_test(NAME osr_early_reflections_tests COMMAND osr_early_reflections_tests)
endif()

if(OSR_BUILD_VST3)
    if(OSR_FETCH_JUCE)
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.6
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(JUCE)
    elseif(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
        add_subdirectory("${JUCE_DIR}" JUCE)
    else()
        find_package(JUCE CONFIG QUIET)
    endif()

    if(TARGET juce::juce_audio_utils)
        juce_add_plugin(OrchestralStudioReverb
            COMPANY_NAME "Orchestral Studio Reverb"
            BUNDLE_ID "org.orchestralstudioreverb.plugin"
            PLUGIN_MANUFACTURER_CODE Osrv
            PLUGIN_CODE Osr1
            FORMATS VST3 Standalone
            PRODUCT_NAME "Orchestral Studio Reverb"
            IS_SYNTH FALSE
            NEEDS_MIDI_INPUT FALSE
            NEEDS_MIDI_OUTPUT FALSE
            IS_MIDI_EFFECT FALSE
            EDITOR_WANTS_KEYBOARD_FOCUS FALSE
            COPY_PLUGIN_AFTER_BUILD FALSE
        )
        target_sources(OrchestralStudioReverb PRIVATE
            Source/Plugin/PluginProcessor.cpp
            Source/Plugin/PluginEditor.cpp
        )
        target_include_directories(OrchestralStudioReverb PRIVATE Source Source/DSP)
        target_compile_definitions(OrchestralStudioReverb PRIVATE
            JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
        )
        target_link_libraries(OrchestralStudioReverb PRIVATE
            osr_dsp
            juce::juce_audio_utils
            juce::juce_dsp
            PUBLIC juce::juce_recommended_config_flags
                   juce::juce_recommended_lto_flags
                   juce::juce_recommended_warning_flags
        )
    else()
        message(STATUS "JUCE not found: building standalone DSP library/tests only. Set JUCE_DIR or enable OSR_FETCH_JUCE.")
    endif()
    if(TARGET juce::juce_gui_extra)
    # Build the orchestra selector in isolation before integrating it into the plugin.
    juce_add_gui_app(OrchestraLayoutPreview
        PRODUCT_NAME "Orchestra Layout Preview"
    )
    target_sources(OrchestraLayoutPreview PRIVATE
        Source/Preview/PreviewMain.cpp
        Source/UI/OrchestraLayoutComponent.cpp
    )
    target_include_directories(OrchestraLayoutPreview PRIVATE Source/UI)
    target_compile_definitions(OrchestraLayoutPreview PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    )
    target_link_libraries(OrchestraLayoutPreview PRIVATE
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )
    endif()

endif()
