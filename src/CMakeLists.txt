# SPDX-FileCopyrightText: 2023 KDE Contributors
#
# SPDX-License-Identifier: LGPL-2.0-or-later

add_subdirectory(ksvg)
add_subdirectory(declarativeimports)

ecm_qt_install_logging_categories(
    EXPORT KSVG
    FILE ksvg.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
