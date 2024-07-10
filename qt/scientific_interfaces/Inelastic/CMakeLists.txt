set(ALL_SRC_FILES)
set(ALL_MOC_FILES)
set(ALL_INC_FILES DllConfig.h PrecompiledHeader.h)
set(ALL_UI_FILES)
set(ALL_RES_FILES Common/InterfaceResources.qrc)

add_subdirectory(BayesFitting)
add_subdirectory(Common)
add_subdirectory(Corrections)
add_subdirectory(Processor)
add_subdirectory(QENSFitting)

# XML is required to parse the settings file
find_package(
  Qt5 ${QT_MIN_VERSION}
  COMPONENTS Concurrent Xml
  REQUIRED
)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesInelastic
  QT_VERSION 5
  SRC ${ALL_SRC_FILES}
  MOC ${ALL_MOC_FILES}
  NOMOC ${ALL_INC_FILES}
  UI ${ALL_UI_FILES}
  RES ${ALL_RES_FILES}
  DEFS IN_MANTIDQT_INELASTIC PRECOMPILED PrecompiledHeader.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBS ${CORE_MANTIDLIBS}
            Mantid::PythonInterfaceCore
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Python::Python
            Qt5::Concurrent
            Qt5::Xml
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsPlotting MantidQtWidgetsMplCpp MantidQtIcons
  INSTALL_DIR ${WORKBENCH_LIB_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../Frameworks @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)

# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
