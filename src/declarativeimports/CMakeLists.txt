ecm_add_qml_module(corebindingsplugin URI "org.kde.ksvg")

target_sources(corebindingsplugin PRIVATE
    corebindingsplugin.cpp
    svgitem.cpp
    framesvgitem.cpp
    managedtexturenode.cpp
    imagetexturescache.cpp
    #quicktheme.cpp
)

target_link_libraries(corebindingsplugin
        Qt6::Quick
        Qt6::Qml
        Qt6::Svg
        KF6::Svg
        KF6::ColorScheme
        KF6::Kirigami2
)

ecm_finalize_qml_module(corebindingsplugin DESTINATION ${KDE_INSTALL_QMLDIR})
