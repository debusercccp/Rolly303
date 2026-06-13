cmake_minimum_required(VERSION 3.22)

project(Rolly303 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Pull in JUCE. By default we download it with FetchContent so the project is
# self-contained. If you already have JUCE checked out locally, point the cache
# variable JUCE_SOURCE_DIR at it and we will use that instead (faster builds):
#   cmake -B build -DJUCE_SOURCE_DIR=/path/to/JUCE
# ---------------------------------------------------------------------------
if(DEFINED JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "Rolly303: using local JUCE at ${JUCE_SOURCE_DIR}")
    add_subdirectory(${JUCE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/JUCE-build)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    message(STATUS "Rolly303: fetching JUCE 8.0.4 from GitHub...")
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Rolly303
    COMPANY_NAME            "Rolly303"
    PLUGIN_MANUFACTURER_CODE Roll
    PLUGIN_CODE             Rl03
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "Rolly303"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       TRUE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Rolly303 PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp)

target_compile_definitions(Rolly303 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

juce_generate_juce_header(Rolly303)

target_link_libraries(Rolly303
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
