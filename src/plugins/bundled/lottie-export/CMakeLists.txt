set(lottie_export_sources
    lottieexportplugin.cpp
    exportlottiedialog.cpp
    lottie/dotlottiewriter.cpp
    lottie/lottieanimatedproperty.cpp
    lottie/lottieblendmode.cpp
    lottie/lottieexporter.cpp
    lottie/lottiejsonoptimizer.cpp
    lottie/lottielayerbuilder.cpp
    lottie/lottieparenting.cpp
    lottie/lottiepatheffects.cpp
    lottie/lottierealkeyframes.cpp
)

add_friction_plugin(friction_lottie_export ${lottie_export_sources})
target_include_directories(friction_lottie_export PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
