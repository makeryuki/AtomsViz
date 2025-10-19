cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

project(AtmosVizCLAP VERSION 0.3.0)

set(PATH_TO_JUCE "C:/juce-8.0.1-windows/JUCE")
set(PATH_TO_CLAP_EXTENSIONS "${CMAKE_SOURCE_DIR}/external/clap-juce-extensions")

if(WIN32)
    set(JUCER_GENERATOR "VisualStudio2022")
else()
    message(FATAL_ERROR "This CMake configuration currently targets Windows builds only.")
endif()

include(${PATH_TO_CLAP_EXTENSIONS}/cmake/JucerClap.cmake)
create_jucer_clap_target(
        TARGET AtmosViz
        PLUGIN_NAME "AtmosViz"
        BINARY_NAME "AtmosViz"
        MANUFACTURER_NAME "AtmosViz"
        MANUFACTURER_URL "https://github.com/makeryuki/AtomsViz"
        VERSION_STRING "0.3.0"
        MANUFACTURER_CODE Manu
        PLUGIN_CODE Dnrt
        CLAP_ID "com.atmosviz.plugin"
        CLAP_FEATURES audio-effect visualizer
        CLAP_MANUAL_URL "https://github.com/makeryuki/AtomsViz"
        CLAP_SUPPORT_URL "https://github.com/makeryuki/AtomsViz/issues"
        EDITOR_NEEDS_KEYBOARD_FOCUS FALSE
)
