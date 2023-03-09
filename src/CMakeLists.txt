add_subdirectory(desktoptheme)
add_subdirectory(plasma)
#add_subdirectory(declarativeimports)
#add_subdirectory(plasmaquick)

ecm_qt_install_logging_categories(
    EXPORT PLASMASVG
    FILE plasmasvg.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
