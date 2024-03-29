set(SRC_FILES FitScriptGenerator.cpp StepScan.cpp)

# Only include header files which do not contain Qt
set(INC_FILES DllConfig.h)

# Only include header files which do contain Qt
set(MOC_FILES FitScriptGenerator.h StepScan.h)

set(UI_FILES StepScan.ui)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesGeneral
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  DEFS IN_MANTIDQT_INTERFACESGENERAL PRECOMPILED PrecompiledHeader.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  SYSTEM_INCLUDE_DIRS ${Boost_INCLUDE_DIRS}
  LINK_LIBS ${CORE_MANTIDLIBS} ${POCO_LIBRARIES} ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} Mantid::Json
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsMplCpp
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../Frameworks
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR}"
)
