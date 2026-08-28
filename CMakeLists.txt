cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for macOS")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(mach1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

add_library(mach1_dsp STATIC
    dsp/MackityEngine.cpp
)
target_include_directories(mach1_dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dsp)
set_target_properties(mach1_dsp PROPERTIES POSITION_INDEPENDENT_CODE ON)

juce_add_plugin(mach1
    COMPANY_NAME "Seto"
    BUNDLE_ID com.seto.mach1
    PLUGIN_MANUFACTURER_CODE Stao
    PLUGIN_CODE Mh01
    FORMATS AU VST3
    PRODUCT_NAME "mach1"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx
)

target_sources(mach1
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginProcessor.h
        src/PluginEditor.cpp
        src/PluginEditor.h
)

target_include_directories(mach1
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/dsp
)

target_compile_definitions(mach1
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(mach1
    PRIVATE
        juce::juce_audio_utils
        mach1_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

enable_testing()

add_executable(mach1_engine_test
    tests/mackity_engine_test.cpp
)
target_include_directories(mach1_engine_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_compile_definitions(mach1_engine_test PRIVATE
    MACH1_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
)
target_link_libraries(mach1_engine_test PRIVATE mach1_dsp)
add_test(NAME mackity_engine COMMAND mach1_engine_test)

add_executable(generate_mackity_fixtures
    tests/generate_mackity_fixtures.cpp
    tests/MackityRef.cpp
)
target_include_directories(generate_mackity_fixtures PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

juce_add_console_app(mach1_passthrough_test
    PRODUCT_NAME "mach1_passthrough_test"
)

target_sources(mach1_passthrough_test
    PRIVATE
        tests/passthrough_test.cpp
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
)

target_include_directories(mach1_passthrough_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/dsp
)

target_compile_definitions(mach1_passthrough_test
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(mach1_passthrough_test
    PRIVATE
        juce::juce_audio_utils
        mach1_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME passthrough COMMAND mach1_passthrough_test)

# Release CPU bar vs compiled Mackity loop. Opponent header is required (fail closed).
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/MackityRef.h")
    message (FATAL_ERROR "Mackity process-loop opponent missing (tests/MackityRef.h); cpu-bench fails closed")
endif()

add_executable(mach1_cpu_bench
    tests/cpu_bench.cpp
    tests/MackityRef.cpp
    dsp/MackityEngine.cpp
)
target_include_directories(mach1_cpu_bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
# Always time Release-level code for this binary (O3 + NDEBUG + LTO).
target_compile_options(mach1_cpu_bench PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-O3>
)
target_compile_definitions(mach1_cpu_bench PRIVATE NDEBUG)
set_property (TARGET mach1_cpu_bench PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
add_test(NAME cpu_bench COMMAND mach1_cpu_bench)
set_tests_properties(cpu_bench PROPERTIES TIMEOUT 300)

# Separate Reaper case: never skip-pass. Missing REAPER.app prints FAIL-UNVERIFIED
# and returns 2 (ctest failure for this test only; cpu_bench above is independent).
add_test(NAME cpu_bench_reaper COMMAND mach1_cpu_bench --reaper)
set_tests_properties(cpu_bench_reaper PROPERTIES TIMEOUT 60)

# Host validation: auval is the required Logic-loadable gate (must exit 0).
# Host-free mono stays on existing ctest 'passthrough' (not duplicated).
# Logic in-app and Reaper VST3 never skip-pass: FAIL-UNVERIFIED → exit 2 on those tests only.
add_test(NAME host_auval
    COMMAND ${CMAKE_COMMAND} -E env
        MACH1_BUILD_DIR=${CMAKE_BINARY_DIR}
        bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/host_validation.sh" --auval
)
set_tests_properties(host_auval PROPERTIES TIMEOUT 180)

add_test(NAME host_logic_e2e
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/host_validation.sh" --logic
)
set_tests_properties(host_logic_e2e PROPERTIES TIMEOUT 30)

add_test(NAME host_reaper_e2e
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/host_validation.sh" --reaper
)
set_tests_properties(host_reaper_e2e PROPERTIES TIMEOUT 30)

add_test(NAME license_attribution
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/license_attribution_test.sh"
)
