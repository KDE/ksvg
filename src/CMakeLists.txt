add_subdirectory(ksvg)
add_subdirectory(declarativeimports)

ecm_qt_install_logging_categories(
    EXPORT KSVG
    FILE ksvg.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
