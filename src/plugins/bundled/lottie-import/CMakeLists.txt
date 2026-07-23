find_package(ZLIB REQUIRED)

add_friction_plugin(friction_lottie_import
    lottieimportplugin.cpp
    lottieimportdialog.cpp
    lottie/lottieimporter.cpp
)
target_include_directories(friction_lottie_import PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(friction_lottie_import PRIVATE ZLIB::ZLIB)
