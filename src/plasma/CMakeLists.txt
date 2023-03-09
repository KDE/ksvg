
if(KDE_PLATFORM_FEATURE_BINARY_COMPATIBLE_FEATURE_REDUCTION)
    set(PLASMA_NO_PACKAGE_EXTRADATA TRUE)
endif()

# Consumer's include dir which has to be explicitly used to make headers of this lib visible to documented includes
# Results in duplicate of prefix-dir & C++ namespace below, but part of different things, so by design:
# <visibility-layer>/<C++ namespace>/class header files
set(PLASMA_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR_KF}/PlasmaSvg")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-plasma.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma.h)

add_library(KF6PlasmaSvg)
add_library(KF6::PlasmaSvg ALIAS KF6PlasmaSvg)

set_target_properties(KF6PlasmaSvg PROPERTIES
    VERSION     ${PLASMA_VERSION}
    SOVERSION   ${PLASMA_SOVERSION}
    EXPORT_NAME PlasmaSvg
)

target_sources(KF6PlasmaSvg PRIVATE
    framesvg.cpp
    svg.cpp
    theme.cpp
    private/theme_p.cpp
)

if(HAVE_X11)
    target_sources(KF6PlasmaSvg PRIVATE private/effectwatcher.cpp)
endif()

kconfig_add_kcfg_files(KF6PlasmaSvg data/kconfigxt/libplasma-theme-global.kcfgc)

ecm_qt_declare_logging_category(KF6PlasmaSvg
    HEADER debug_p.h
    IDENTIFIER LOG_PLASMASVG
    CATEGORY_NAME kf.plasmasvg
    OLD_CATEGORY_NAMES org.kde.plasma
    DESCRIPTION "PlasmaSvg lib"
    EXPORT PLASMASVG
)

ecm_generate_export_header(KF6PlasmaSvg
    EXPORT_FILE_NAME plasma/plasmasvg_export.h
    BASE_NAME PlasmaSvg
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    DEPRECATED_BASE_VERSION 0
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
    DEPRECATION_VERSIONS
)

if(HAVE_X11)
    set(PLASMASVG_EXTRA_LIBS ${PLASMA_EXTRA_LIBS} XCB::XCB Qt6::GuiPrivate)
endif()

target_link_libraries(KF6PlasmaSvg
PUBLIC
    Qt6::Gui
    KF6::ConfigCore
    KF6::CoreAddons
PRIVATE
    Qt6::Svg
    KF6::Archive
    KF6::GuiAddons #kimagecache
    KF6::I18n
    KF6::WindowSystem #compositingActive
    KF6::XmlGui   #KActionCollection
    KF6::GlobalAccel #Applet::setGlobalShortcut
    KF6::Notifications
    KF6::IconThemes
    ${PLASMASVG_EXTRA_LIBS}
)

target_link_libraries(KF6PlasmaSvg PRIVATE KF6::Package)

set(Plasma_BUILD_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/PlasmaSvg
)
target_include_directories(KF6PlasmaSvg
    PUBLIC
        "$<BUILD_INTERFACE:${Plasma_BUILD_INCLUDE_DIRS}>"
    INTERFACE
        "$<INSTALL_INTERFACE:${PLASMA_INSTALL_INCLUDEDIR}>"
)

########### install files ###############
ecm_generate_headers(Plasma_CamelCase_HEADERS
    HEADER_NAMES
        FrameSvg
        Svg
        Theme
    REQUIRED_HEADERS PlasmaSvg_HEADERS
    PREFIX PlasmaSvg
)

install(FILES
        ${Plasma_HEADERS}
        ${CMAKE_CURRENT_BINARY_DIR}/plasmasvg/plasmasvg_export.h
    DESTINATION ${PLASMA_INSTALL_INCLUDEDIR}/plasmasvg # C++ namespace
    COMPONENT Devel
)

install(
    FILES ${Plasma_CamelCase_HEADERS}
    DESTINATION ${PLASMA_INSTALL_INCLUDEDIR}/PlasmaSvg # C++ namespace
    COMPONENT Devel
)

install(TARGETS KF6PlasmaSvg EXPORT KF6PlasmaSvgTargets ${KF_INSTALL_TARGETS_DEFAULT_ARGS})

if(BUILD_QCH)
    ecm_add_qch(
        KF6PlasmaSvg_QCH
        NAME Plasma
        BASE_NAME KF6PlasmaSvg
        VERSION ${KF_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${Plasma_HEADERS}
            Mainpage.dox
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt6Gui_QCH
        INCLUDE_DIRS
            ${Plasma_BUILD_INCLUDE_DIRS}
        BLANK_MACROS
            PLASMA_EXPORT
            PLASMA_DEPRECATED
            PLASMA_DEPRECATED_EXPORT
            "PLASMA_DEPRECATED_VERSION(x, y, t)"
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()
