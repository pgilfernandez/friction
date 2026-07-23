cmake_minimum_required(VERSION 3.12)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
find_package(PkgConfig QUIET)
include(friction-ffmpeg)

find_package(Qt5 5.15.13 COMPONENTS
    Core Gui Widgets Xml Qml OpenGL Multimedia REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(frictionpluginapi INTERFACE)
target_include_directories(frictionpluginapi INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/api>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ui>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../skia>
    $<INSTALL_INTERFACE:include/friction>
    $<INSTALL_INTERFACE:include/friction/core>
    $<INSTALL_INTERFACE:include/friction/ui>
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_directories(frictionpluginapi INTERFACE ${FFMPEG_LIBRARIES_DIRS})

if(WIN32)
    target_link_directories(frictionpluginapi INTERFACE ${CMAKE_SOURCE_DIR}/sdk/bin)
    set(FRICTION_PLUGIN_SKIA_LIBRARIES skia user32 opengl32)
elseif(SKIA_STATIC)
    target_link_directories(frictionpluginapi INTERFACE ${CMAKE_BINARY_DIR}/src/skia)
    set(FRICTION_PLUGIN_SKIA_LIBRARIES skia)
else()
    target_link_directories(frictionpluginapi INTERFACE ${CMAKE_BINARY_DIR}/src/skia)
    set(FRICTION_PLUGIN_SKIA_LIBRARIES skia-friction)
endif()

target_link_libraries(frictionpluginapi INTERFACE
    frictioncore
    frictionui
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Xml
    Qt5::Qml
    Qt5::OpenGL
    Qt5::Multimedia
    ${FFMPEG_LIBRARIES}
    ${FRICTION_PLUGIN_SKIA_LIBRARIES}
)

include(GNUInstallDirs)
install(FILES api/frictionplugin.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/friction)
install(DIRECTORY ../core/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/friction/core
        FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ../ui/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/friction/ui
        FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ../skia/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/friction/include
        FILES_MATCHING PATTERN "*.h")

function(add_friction_plugin target)
    add_library(${target} MODULE ${ARGN})
    target_link_libraries(${target} PRIVATE frictionpluginapi)
    set_target_properties(${target} PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/app/plugins"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/app/plugins"
    )
    if(APPLE)
        set_target_properties(${target} PROPERTIES
            BUILD_RPATH "${CMAKE_BINARY_DIR}/src/core;${CMAKE_BINARY_DIR}/src/ui"
            INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../../lib"
        )
        set(plugin_destination "friction.app/Contents/PlugIns")
    elseif(WIN32)
        set(plugin_destination "plugins")
    else()
        set_target_properties(${target} PROPERTIES INSTALL_RPATH "$ORIGIN/../..")
        set(plugin_destination "${CMAKE_INSTALL_LIBDIR}/friction/plugins")
    endif()
    install(TARGETS ${target}
            LIBRARY DESTINATION "${plugin_destination}"
            RUNTIME DESTINATION "${plugin_destination}")
endfunction()

if(BUILD_BUNDLED_PLUGINS)
    add_subdirectory(bundled/lottie-export)
endif()
