add_subdirectory(desktoptheme)
add_subdirectory(plasmasvg)
add_subdirectory(declarativeimports)

ecm_qt_install_logging_categories(
    EXPORT PLASMASVG
    FILE plasmasvg.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
