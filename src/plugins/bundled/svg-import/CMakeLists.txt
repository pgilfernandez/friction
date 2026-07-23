add_friction_plugin(friction_svg_import
    svgimportplugin.cpp
    svganimationimportdialog.cpp
    svganimationimporter.cpp
)
target_include_directories(friction_svg_import PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
