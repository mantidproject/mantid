set(ALL_SRC_FILES)
set(ALL_MOC_FILES)
set(ALL_INC_FILES DllConfig.h PrecompiledHeader.h)
set(ALL_UI_FILES)

add_subdirectory(Common)
add_subdirectory(Diffraction)
add_subdirectory(Reduction)
add_subdirectory(Simulation)
add_subdirectory(Tools)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesIndirect
  QT_VERSION 5
  SRC ${ALL_SRC_FILES}
  MOC ${ALL_MOC_FILES}
  NOMOC ${ALL_INC_FILES}
  UI ${ALL_UI_FILES}
  DEFS IN_MANTIDQT_INDIRECT PRECOMPILED PrecompiledHeader.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBS ${CORE_MANTIDLIBS} Mantid::PythonInterfaceCore ${POCO_LIBRARIES} ${Boost_LIBRARIES} Python::Python
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsPlotting MantidQtWidgetsMplCpp MantidQtWidgetsSpectroscopy
                   MantidQtIcons
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../Frameworks @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)

# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
