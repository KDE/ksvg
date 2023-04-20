find_package(Qt6Test ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)
set_package_properties(Qt6Test PROPERTIES PURPOSE "Required for tests")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_STRICT_ITERATORS -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_KEYWORDS)

include(ECMMarkAsTest)
include(ECMAddTests)

# add_definitions( -DKDESRCDIR=${CMAKE_CURRENT_SOURCE_DIR} )

if(KDE_PLATFORM_FEATURE_BINARY_COMPATIBLE_FEATURE_REDUCTION)
    set(KSVG_NO_PACKAGE_EXTRADATA TRUE)
endif()

MACRO(KSVG_UNIT_TESTS)
       FOREACH(_testname ${ARGN})
               set(libs Qt6::Qml Qt6::Test KF6::Svg
                        KF6::Archive KF6::CoreAddons KF6::ConfigGui)
               if(QT_QTOPENGL_FOUND)
                   list(APPEND libs Qt6::OpenGL)
               endif()
               ecm_add_test(${_testname}.cpp
                            LINK_LIBRARIES ${libs}
                            NAME_PREFIX "plasmasvg-")
               target_include_directories(${_testname} PRIVATE )
       ENDFOREACH(_testname)
ENDMACRO(KSVG_UNIT_TESTS)

KSVG_UNIT_TESTS(
    framesvgtest
)


#Add a test that i18n is not used directly in any import.
# It should /always/ be i18nd
find_program(SH bash)
if(SH)
    add_test(i18ndcheck ${SH} ${CMAKE_CURRENT_SOURCE_DIR}/i18ndcheck.sh ${CMAKE_SOURCE_DIR}/src/declarativeimports)
endif()
