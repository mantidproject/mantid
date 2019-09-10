function(
  PREPEND
  var
  prefix
)
  set(listVar "")
  foreach(
    f
    ${ARGN}
  )
    list(
      APPEND
        listVar
        "${prefix}/${f}"
    )
  endforeach(f)
  set(${var} ${listVar} PARENT_SCOPE)
endfunction(PREPEND)

add_subdirectory(Common)
add_subdirectory(GUI)
add_subdirectory(Reduction)
add_subdirectory(TestHelpers)

set(
  SRC_FILES
  ${COMMON_SRC_FILES}
  ${GUI_SRC_FILES}
  ${REDUCTION_SRC_FILES}
  ${TESTHELPERS_SRC_FILES}
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(
  INC_FILES
  ${COMMON_INC_FILES}
  ${GUI_INC_FILES}
  ${REDUCTION_INC_FILES}
  ${TESTHELPERS_INC_FILES}
  PrecompiledHeader.h
)

set(MOC_FILES ${GUI_MOC_FILES})

set(UI_FILES ${GUI_UI_FILES})

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesISISReflectometry
  QT_VERSION 4
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  DEFS
    IN_MANTIDQT_ISISREFLECTOMETRY
    PRECOMPILED
    PrecompiledHeader.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${JSONCPP_LIBRARIES}
  QT4_LINK_LIBS Qwt5
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtIcons
  INSTALL_DIR_BASE ${PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../Contents/MacOS
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR}"
)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesISISReflectometry
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  DEFS
    IN_MANTIDQT_ISISREFLECTOMETRY
    PRECOMPILED
    PrecompiledHeader.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${PYTHON_LIBRARIES}
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtIcons
    MantidQtWidgetsMplCpp
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../../Contents/MacOS
    @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR}"
)
