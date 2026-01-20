set(SRC_FILES
    ALFAlgorithmManager.cpp
    ALFAnalysisModel.cpp
    ALFAnalysisPresenter.cpp
    ALFAnalysisView.cpp
    ALFInstrumentModel.cpp
    ALFInstrumentPresenter.cpp
    ALFInstrumentView.cpp
    ALFInstrumentWidget.cpp
    ALFView.cpp
)

set(MOC_FILES ALFAnalysisView.h ALFInstrumentView.h ALFInstrumentWidget.h ALFView.h)

set(INC_FILES
    ALFAlgorithmManager.h
    ALFAnalysisModel.h
    ALFAnalysisPresenter.h
    ALFData.h
    ALFInstrumentModel.h
    ALFInstrumentPresenter.h
    DetectorTube.h
    IALFAlgorithmManagerSubscriber.h
)

# Target
mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesDirect
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  DEFS IN_MANTIDQT_DIRECT
  LINK_LIBS Mantid::API Mantid::DataObjects Mantid::Geometry Mantid::Kernel Mantid::PythonInterfaceCore Python::Python
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsInstrumentView MantidQtWidgetsPlotting MantidQtWidgetsMplCpp
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)

# Auto-generate exports header
target_include_directories(
  MantidScientificInterfacesDirect-Qt5 PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/qt/scientific_interfaces/Direct> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(MantidScientificInterfacesDirect TRUE)

# Testing target We can currently only include the tests if we are building the framework. This is bceause we depend on
# a non-exported library of frameworks in the tests.
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
