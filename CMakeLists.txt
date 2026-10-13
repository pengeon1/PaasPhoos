cmake_minimum_required(VERSION 3.22)

project(PaasPhoos VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Support Steinberg ASIO SDK if present, otherwise WASAPI fallback
set(ASIOSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/asiosdk" CACHE PATH "Path to Steinberg ASIO SDK")
if(EXISTS "${ASIOSDK_DIR}/iasiodrv.h" OR EXISTS "${ASIOSDK_DIR}/common/iasiodrv.h")
    message(STATUS "Found Steinberg ASIO SDK at: ${ASIOSDK_DIR}")
    set(PAASPHOOS_ENABLE_ASIO ON)
else()
    message(STATUS "Steinberg ASIO SDK not present in external/asiosdk. Building with Windows WASAPI engine.")
    set(PAASPHOOS_ENABLE_ASIO OFF)
endif()

# Include JUCE 8
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/juce/CMakeLists.txt")
    add_subdirectory(external/juce)
else()
    include(FetchContent)
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(juce)
endif()

# Define GUI Application
juce_add_gui_app(PaasPhoos
    PRODUCT_NAME "PaasPhoos"
    VERSION "0.1.0"
    COMPANY_NAME "PaasPhoos"
)

target_sources(PaasPhoos PRIVATE
    src/Main.cpp
    src/audio/AudioEngine.h
    src/audio/AudioEngine.cpp
    src/audio/Metronome.h
    src/audio/Tuner.h
    src/audio/RealtimeRingBuffer.h
    src/audio/BuiltInFxChain.h
    src/audio/BuiltInFxChain.cpp
    src/audio/PluginHostManager.h
    src/audio/PluginHostManager.cpp
    src/audio/DrumEngine.h
    src/audio/DrumEngine.cpp
    src/audio/DrumSequencer.h
    src/audio/DrumSequencer.cpp
    src/audio/SessionRecorder.h
    src/audio/SessionRecorder.cpp
    src/session/SetlistManager.h
    src/session/SetlistManager.cpp
    src/session/ChatTalkback.h
    src/session/ChatTalkback.cpp
    src/network/NetworkPacket.h
    src/network/NetworkStats.h
    src/network/NetworkAdapterDetector.h
    src/network/NetworkAdapterDetector.cpp
    src/network/NetworkSimulator.h
    src/network/StunClient.h
    src/network/StunClient.cpp
    src/network/JitterBuffer.h
    src/network/JitterBuffer.cpp
    src/network/LanDiscovery.h
    src/network/LanDiscovery.cpp
    src/network/LanAudioTransport.h
    src/network/LanAudioTransport.cpp
    src/ui/MainComponent.h
    src/ui/MainComponent.cpp
    src/ui/SessionPanel.h
    src/ui/SessionPanel.cpp
    src/ui/AudioSettingsPanel.h
    src/ui/AudioSettingsPanel.cpp
    src/ui/MixerPanel.h
    src/ui/MixerPanel.cpp
    src/ui/TunerComponent.h
    src/ui/TunerComponent.cpp
    src/ui/MetronomeComponent.h
    src/ui/MetronomeComponent.cpp
    src/ui/EffectsPanel.h
    src/ui/EffectsPanel.cpp
    src/ui/DrumPanel.h
    src/ui/DrumPanel.cpp
    src/ui/RehearsalToolsPanel.h
    src/ui/RehearsalToolsPanel.cpp
    src/ui/IntroScreenComponent.h
    src/ui/IntroScreenComponent.cpp
    src/common/LookAndFeelCustom.h
)

target_include_directories(PaasPhoos PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(PAASPHOOS_ENABLE_ASIO)
    target_compile_definitions(PaasPhoos PRIVATE JUCE_ASIO=1)
    if(EXISTS "${ASIOSDK_DIR}/common")
        target_include_directories(PaasPhoos PRIVATE "${ASIOSDK_DIR}/common" "${ASIOSDK_DIR}/host/pc")
    else()
        target_include_directories(PaasPhoos PRIVATE "${ASIOSDK_DIR}")
    endif()
else()
    target_compile_definitions(PaasPhoos PRIVATE JUCE_ASIO=0)
endif()

target_compile_definitions(PaasPhoos PRIVATE
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    NOMINMAX=1
    WIN32_LEAN_AND_MEAN=1
)

target_link_libraries(PaasPhoos PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
)

if(WIN32)
    target_link_libraries(PaasPhoos PRIVATE avrt winmm iphlpapi ws2_32)
endif()

# Headless Test-Bot Client
add_executable(PaasPhoosBot
    tools/TestBot.cpp
)

target_include_directories(PaasPhoosBot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(PaasPhoosBot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    NOMINMAX=1
    WIN32_LEAN_AND_MEAN=1
)

target_link_libraries(PaasPhoosBot PRIVATE
    juce::juce_core
)

if(WIN32)
    target_link_libraries(PaasPhoosBot PRIVATE avrt winmm iphlpapi ws2_32)
endif()

# Unit Tests Executable
add_executable(PaasPhoosTests
    tests/TestMain.cpp
    src/network/JitterBuffer.cpp
    src/network/StunClient.cpp
    src/audio/BuiltInFxChain.cpp
    src/audio/DrumEngine.cpp
    src/audio/DrumSequencer.cpp
    src/session/SetlistManager.cpp
)

target_include_directories(PaasPhoosTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(PaasPhoosTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    NOMINMAX=1
    WIN32_LEAN_AND_MEAN=1
)

target_link_libraries(PaasPhoosTests PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_data_structures
)

if(WIN32)
    target_link_libraries(PaasPhoosTests PRIVATE avrt winmm iphlpapi ws2_32)
endif()

enable_testing()
add_test(NAME PaasPhoosAudioTests COMMAND PaasPhoosTests)

