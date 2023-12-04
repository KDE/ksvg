find_package(Qt6Test ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)
set_package_properties(Qt6Test PROPERTIES PURPOSE "Required for tests")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
remove_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_STRICT_ITERATORS
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_KEYWORDS
)

include(ECMMarkAsTest)
include(ECMAddTests)

if(KDE_PLATFORM_FEATURE_BINARY_COMPATIBLE_FEATURE_REDUCTION)
    set(KSVG_NO_PACKAGE_EXTRADATA TRUE)
endif()

macro(ksvg_unit_tests)
    foreach(_targetname ${ARGN})
        set(_testname "plasmasvg-${_targetname}")
        set(libs
            Qt6::Qml
            Qt6::Quick
            Qt6::Test
            KF6::Svg
            KF6::Archive
            KF6::CoreAddons
            KF6::ConfigGui
        )
        if(QT_QTOPENGL_FOUND)
            list(APPEND libs Qt6::OpenGL)
        endif()

        set(_extra_target_args WIN32 MACOSX_BUNDLE)
        set(_extra_run_args)
        if (WIN32)
            set(_extra_run_args -platform offscreen)
        endif()

        add_executable(${_targetname} ${_extra_target_args} ${_targetname}.cpp)
        target_link_libraries(${_targetname} PRIVATE ${libs})
        add_test(
            NAME ${_testname}
            COMMAND
                ${_targetname}
                ${_extra_args}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endforeach()
endmacro()

ksvg_unit_tests(
    framesvgtest
    framesvgitemtest
    svgitemtest
)
