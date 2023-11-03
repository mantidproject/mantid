set(SRC_FILES
    Analysis/ConvFitAddWorkspaceDialog.cpp
    Analysis/ConvFitDataView.cpp
    Analysis/ConvFitDataPresenter.cpp
    Analysis/IndirectDataAnalysis.cpp
    Analysis/IndirectDataAnalysisTab.cpp
    Analysis/IndirectDataAnalysisMSDFitTab.cpp
    Analysis/IndirectDataAnalysisIqtFitTab.cpp
    Analysis/IndirectDataAnalysisConvFitTab.cpp
    Analysis/IndirectDataAnalysisFqFitTab.cpp
    Analysis/FqFitAddWorkspacedialog.cpp
    Analysis/FqFitDataPresenter.cpp
    Analysis/FqFitDataView.cpp
    Analysis/FqFitModel.cpp
    Analysis/IqtFitModel.cpp
    Analysis/MSDFitModel.cpp
)

set(MOC_FILES
    Analysis/ConvFitAddWorkspaceDialog.h
    Analysis/ConvFitDataView.h
    Analysis/ConvFitDataPresenter.h
    Analysis/IndirectDataAnalysis.h
    Analysis/IndirectDataAnalysisTab.h
    Analysis/IndirectDataAnalysisMSDFitTab.h
    Analysis/IndirectDataAnalysisIqtFitTab.h
    Analysis/IndirectDataAnalysisConvFitTab.h
    Analysis/IndirectDataAnalysisFqFitTab.h
    Analysis/FqFitAddWorkspacedialog.h
    Analysis/FqFitDataPresenter.h
    Analysis/FqFitDataView.h
)

set(INC_FILES DllConfig.h PrecompiledHeader.h Analysis/FqFitModel.h Analysis/IqtFitModel.h Analysis/MSDFitModel.h)

set(UI_FILES Analysis/ConvFitAddWorkspaceDialog.ui Analysis/FqFitAddWorkspaceDialog.ui Analysis/IndirectDataAnalysis.ui)

set(RES_FILES ../Indirect/IndirectInterfaceResources.qrc)

# XML is required to parse the settings file
find_package(
  Qt5 ${QT_MIN_VERSION}
  COMPONENTS Concurrent Xml
  REQUIRED
)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesInelastic
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  RES ${RES_FILES}
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
                   MantidScientificInterfacesIndirect
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../Frameworks @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)

# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
