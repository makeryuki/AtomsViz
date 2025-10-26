cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

project(AtmosVizCLAP VERSION 0.4.0)

set(PATH_TO_JUCE "" CACHE PATH "Path to the JUCE checkout")
set(PATH_TO_CLAP_EXTENSIONS "${CMAKE_SOURCE_DIR}/external/clap-juce-extensions" CACHE PATH "Path to clap-juce-extensions")

if(NOT PATH_TO_JUCE)
    if(DEFINED ENV{JUCE_PATH})
        set(PATH_TO_JUCE "$ENV{JUCE_PATH}")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/../JUCE")
        set(PATH_TO_JUCE "${CMAKE_SOURCE_DIR}/../JUCE")
    else()
        message(FATAL_ERROR "PATH_TO_JUCE is not set. Provide it via -DPATH_TO_JUCE=/path/to/JUCE or set the JUCE_PATH environment variable.")
    endif()
endif()

if(WIN32)
    set(JUCER_GENERATOR "VisualStudio2022")
elseif(APPLE)
    set(JUCER_GENERATOR "Xcode")
else()
    message(FATAL_ERROR "This CMake configuration currently targets Windows and macOS builds only.")
endif()

include(${PATH_TO_CLAP_EXTENSIONS}/cmake/JucerClap.cmake)
create_jucer_clap_target(
        TARGET AtmosViz
        PLUGIN_NAME "AtmosViz"
        BINARY_NAME "AtmosViz"
        MANUFACTURER_NAME "AtmosViz"
        MANUFACTURER_URL "https://github.com/makeryuki/AtomsViz"
        VERSION_STRING "0.4.0"
        MANUFACTURER_CODE Manu
        PLUGIN_CODE Dnrt
        CLAP_ID "com.atmosviz.plugin"
        CLAP_FEATURES audio-effect visualizer
        CLAP_MANUAL_URL "https://github.com/makeryuki/AtomsViz"
        CLAP_SUPPORT_URL "https://github.com/makeryuki/AtomsViz/issues"
        EDITOR_NEEDS_KEYBOARD_FOCUS FALSE
)
