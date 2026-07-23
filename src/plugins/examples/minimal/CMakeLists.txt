cmake_minimum_required(VERSION 3.12)
project(friction_minimal_plugin LANGUAGES CXX)

set(FRICTION_PLUGIN_API_DIR "" CACHE PATH
    "Directory containing frictionplugin.h")
if(NOT EXISTS "${FRICTION_PLUGIN_API_DIR}/frictionplugin.h")
    message(FATAL_ERROR "Set FRICTION_PLUGIN_API_DIR to Friction's src/plugins/api")
endif()

find_package(Qt5 5.15.13 COMPONENTS Core Gui Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(friction_minimal_plugin MODULE minimalplugin.cpp)
target_include_directories(friction_minimal_plugin PRIVATE
    "${FRICTION_PLUGIN_API_DIR}")
target_link_libraries(friction_minimal_plugin PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets)
set_target_properties(friction_minimal_plugin PROPERTIES PREFIX "")
