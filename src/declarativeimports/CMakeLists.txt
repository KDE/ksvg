ecm_add_qml_module(corebindingsplugin
    URI "org.kde.ksvg"
    VERSION 1.0
    DEPENDENCIES
        QtQuick
    GENERATE_PLUGIN_SOURCE
)

target_sources(corebindingsplugin PRIVATE
    framesvgitem.cpp
    imagetexturescache.cpp
    managedtexturenode.cpp
    svgitem.cpp
    types.h
)

target_link_libraries(corebindingsplugin PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Svg
    KF6::Svg
    KF6::ColorScheme
    KF6::KirigamiPlatform
)

ecm_qt_declare_logging_category(corebindingsplugin
    HEADER debug_p.h
    IDENTIFIER LOG_KSVGQML
    CATEGORY_NAME kf.svg
    DESCRIPTION "KSvg QML plugin"
)

ecm_finalize_qml_module(corebindingsplugin DESTINATION ${KDE_INSTALL_QMLDIR})
