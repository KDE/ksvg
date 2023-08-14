# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

find_library(resvg_LIBRARY resvg)
find_path(resvg_INCLUDE_DIR NAMES resvg.h)

if(resvg_LIBRARY AND resvg_INCLUDE_DIR)
    add_library(resvg::resvg SHARED IMPORTED)
    set_target_properties(resvg::resvg PROPERTIES
        IMPORTED_LOCATION "${resvg_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${resvg_INCLUDE_DIR}"
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(resvg
    FOUND_VAR resvg_FOUND
    REQUIRED_VARS resvg_LIBRARY resvg_INCLUDE_DIR
)
