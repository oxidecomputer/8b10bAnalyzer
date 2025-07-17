cmake_minimum_required (VERSION 3.13)

project(decode8b10bAnalyzer)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/decode8b10bAnalyzer.cpp
src/decode8b10bAnalyzer.h
src/decode8b10bAnalyzerResults.cpp
src/decode8b10bAnalyzerResults.h
src/decode8b10bAnalyzerSettings.cpp
src/decode8b10bAnalyzerSettings.h
src/decode8b10bSimulationDataGenerator.cpp
src/decode8b10bSimulationDataGenerator.h
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})
