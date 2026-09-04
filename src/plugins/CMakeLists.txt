cmake_minimum_required(VERSION 3.12)

find_package(Qt5 5.15.13 COMPONENTS
    Core Gui Widgets Xml Qml OpenGL Multimedia REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(friction_svg_import MODULE
    bundled/svg-import/svgimportplugin.cpp
    bundled/svg-import/svganimationimportdialog.cpp
    bundled/svg-import/svganimationimporter.cpp
)

target_include_directories(friction_svg_import PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/bundled/svg-import
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../skia
    $<TARGET_PROPERTY:frictioncore,INCLUDE_DIRECTORIES>
)

target_link_libraries(friction_svg_import PRIVATE
    frictioncore
    frictionui
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Xml
    Qt5::Qml
    Qt5::OpenGL
    Qt5::Multimedia
)

target_link_directories(friction_svg_import PRIVATE
    $<TARGET_PROPERTY:frictioncore,LINK_DIRECTORIES>
)
if(WIN32)
    target_link_libraries(friction_svg_import PRIVATE skia user32 opengl32)
else()
    target_link_libraries(friction_svg_import PRIVATE skia-friction)
endif()

set_target_properties(friction_svg_import PROPERTIES PREFIX "")
if(APPLE)
    set_target_properties(friction_svg_import PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/src/app/friction.app/Contents/MacOS/plugins"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/src/core;${CMAKE_BINARY_DIR}/src/ui"
        INSTALL_RPATH "@loader_path/../../Frameworks"
    )
    set(FRICTION_PLUGIN_INSTALL_DIR
        "friction.app/Contents/MacOS/plugins")
elseif(WIN32)
    set_target_properties(friction_svg_import PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/app/plugins"
    )
    set(FRICTION_PLUGIN_INSTALL_DIR "plugins")
else()
    set_target_properties(friction_svg_import PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/app/plugins"
        INSTALL_RPATH "$ORIGIN/../../lib"
    )
    include(GNUInstallDirs)
    set(FRICTION_PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}/plugins")
endif()

install(TARGETS friction_svg_import
    LIBRARY DESTINATION "${FRICTION_PLUGIN_INSTALL_DIR}"
    RUNTIME DESTINATION "${FRICTION_PLUGIN_INSTALL_DIR}"
)

add_custom_target(all_friction_plugins DEPENDS friction_svg_import)

if(BUILD_TESTING)
    add_executable(friction_core_plugin_probe pluginprobe.cpp)
    target_include_directories(friction_core_plugin_probe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../core
    )
    target_link_libraries(friction_core_plugin_probe PRIVATE
        frictioncore Qt5::Core Qt5::Widgets
    )
endif()
