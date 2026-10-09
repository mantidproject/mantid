set(SRC_FILES
    ALFAlgorithmManager.cpp
    ALFAnalysisModel.cpp
    ALFAnalysisPresenter.cpp
    ALFAnalysisView.cpp
    ALFInstrumentModel.cpp
    ALFInstrumentPresenter.cpp
    ALFInstrumentView.cpp
    ALFPythonInstrumentView.cpp
    ALFInstrumentViewBase.cpp
    ALFInstrumentWidget.cpp
    ALFView.cpp
)

set(MOC_FILES
    ALFAnalysisView.h
    ALFInstrumentView.h
    ALFPythonInstrumentView.h
    ALFPythonCallbackRelay.h
    ALFInstrumentViewBase.h
    ALFInstrumentWidget.h
    ALFView.h
)

set(INC_FILES
    ALFAlgorithmManager.h
    ALFAnalysisModel.h
    ALFAnalysisPresenter.h
    ALFData.h
    ALFInstrumentModel.h
    ALFInstrumentPresenter.h
    DetectorTube.h
    ALFPythonCallbackRelay.h
    IALFAlgorithmManagerSubscriber.h
    IALFInstrumentView.h
)

# Target
mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesDirect
  QT_VERSION ${MANTID_QT_VERSION}
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
  LINK_LIBS Mantid::API Mantid::DataObjects Mantid::Geometry Mantid::Kernel Mantid::PythonInterfaceCore
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsInstrumentView MantidQtWidgetsPlotting MantidQtWidgetsMplCpp
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../Frameworks @loader_path/../../plugins/qt6
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt6/"
)
# Testing target We can currently only include the tests if we are building the framework. This is bceause we depend on
# a non-exported library of frameworks in the tests.
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()

# Auto-generate exports header
generate_mantid_export_header(
  MantidScientificInterfacesDirectQt${MANTID_QT_VERSION}
  TRUE
  BASE_NAME
  MANTIDQT_DIRECT
  EXPORT_MACRO_NAME
  MANTIDQT_DIRECT_DLL
  EXPORT_FILE_NAME
  DllConfig.h
)
