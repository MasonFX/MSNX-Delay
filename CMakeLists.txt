cmake_minimum_required(VERSION 3.22)

# Mac: one build that runs on both Apple Silicon and Intel Macs, macOS 11 or newer.
# (Must be set before project().)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Mac CPU types to build for")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Oldest macOS supported")

# Windows: build the C++ runtime into the plugin, so your friend
# doesn't need to install the "Visual C++ Redistributable".
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Mpc3kDelay VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Plugin formats: VST3 + a standalone app everywhere, plus Audio Unit on Mac (for Logic / GarageBand).
set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

# --- JUCE --------------------------------------------------------------------
# Downloads JUCE 9.0.4 the first time you configure.
# Already have JUCE on disk? Point to it instead and skip the download:
#   cmake -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=C:/path/to/JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# --- The delay core (plain C++, no JUCE) -------------------------------------
add_library(mpc3k_core STATIC core/Mpc3kDelay.cpp)
target_include_directories(mpc3k_core PUBLIC core)

# --- The plugin --------------------------------------------------------------
juce_add_plugin(Mpc3kDelay
    COMPANY_NAME            "K"
    PRODUCT_NAME            "MSNX Delay"
    PLUGIN_MANUFACTURER_CODE Kkkk
    PLUGIN_CODE             M3kd
    FORMATS                 ${PLUGIN_FORMATS}
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(Mpc3kDelay PRIVATE
    plugin/PluginProcessor.cpp
    plugin/PluginEditor.cpp
)

target_compile_definitions(Mpc3kDelay PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Mpc3kDelay
    PRIVATE
        mpc3k_core
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# --- The core tests ----------------------------------------------------------
enable_testing()
add_executable(core_test tests/core_test.cpp)
target_link_libraries(core_test PRIVATE mpc3k_core)
add_test(NAME core_test COMMAND core_test)

add_executable(lfo_tempo_test tests/lfo_tempo_test.cpp)
target_link_libraries(lfo_tempo_test PRIVATE mpc3k_core)
add_test(NAME lfo_tempo_test COMMAND lfo_tempo_test)
