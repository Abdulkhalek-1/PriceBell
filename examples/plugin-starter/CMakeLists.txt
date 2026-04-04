cmake_minimum_required(VERSION 3.16)
project(ExamplePlugin VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Point to the PriceBell source tree so we can include IPlugin2.hpp, DataStructs.hpp, etc.
# Adjust PRICEBELL_SOURCE_DIR if building outside the main repo.
if(NOT DEFINED PRICEBELL_SOURCE_DIR)
    set(PRICEBELL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

add_library(ExamplePlugin SHARED
    ExamplePlugin.hpp
    ExamplePlugin.cpp
    example-plugin.json
)

target_include_directories(ExamplePlugin PRIVATE
    "${PRICEBELL_SOURCE_DIR}/include"
)

target_link_libraries(ExamplePlugin Qt5::Core Qt5::Widgets)

# Output the plugin into a 'plugins/' directory for easy testing
set_target_properties(ExamplePlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)
