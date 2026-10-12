set(SRC_FILES FitScriptGenerator.cpp StepScan.cpp)

# Only include header files which do contain Qt
set(MOC_FILES FitScriptGenerator.h StepScan.h)

set(UI_FILES StepScan.ui)

set(PRECOMP_HEADERS
    <MantidAPI/Algorithm.h>
    <MantidAPI/MatrixWorkspace_fwd.h>
    <MantidKernel/System.h>
    <QDialog>
    <QString>
    <QWidget>
    <map>
    <set>
    <string>
    <vector>
)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesGeneral
  QT_VERSION ${MANTID_QT_VERSION}
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  UI ${UI_FILES}
  DEFS IN_MANTIDQT_INTERFACESGENERAL PRECOMPILED ${PRECOMP_HEADERS}
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
  SYSTEM_INCLUDE_DIRS ${Boost_INCLUDE_DIRS}
  LINK_LIBS ${CORE_MANTIDLIBS} ${POCO_LIBRARIES} ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} Mantid::Json
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsMplCpp
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../Frameworks
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR}"
)

# Auto-generate exports header
generate_mantid_export_header(
  MantidScientificInterfacesGeneralQt${MANTID_QT_VERSION}
  TRUE
  BASE_NAME
  MANTIDQT_INTERFACESGENERAL
  EXPORT_MACRO_NAME
  MANTIDQT_INTERFACESGENERAL_DLL
  EXPORT_FILE_NAME
  DllConfig.h
)
