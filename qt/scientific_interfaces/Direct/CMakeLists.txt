set(SRC_FILES
    ALFAnalysisModel.cpp
    ALFAnalysisPresenter.cpp
    ALFAnalysisView.cpp
    ALFInstrumentModel.cpp
    ALFInstrumentPresenter.cpp
    ALFInstrumentView.cpp
    ALFView.cpp
)

set(MOC_FILES ALFAnalysisView.h ALFInstrumentView.h ALFView.h)

set(INC_FILES
    ALFAnalysisMocks.h
    ALFAnalysisModel.h
    ALFAnalysisPresenter.h
    ALFInstrumentMocks.h
    ALFInstrumentModel.h
    ALFInstrumentPresenter.h
    DllConfig.h
)

# Target
mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesDirect
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  DEFS IN_MANTIDQT_DIRECT
  LINK_LIBS Mantid::DataObjects
            ${CORE_MANTIDLIBS}
            Mantid::PythonInterfaceCore
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${PYTHON_LIBRARIES}
            ${OPENGL_gl_LIBRARY}
            ${OPENGL_glu_LIBRARY}
  QT5_LINK_LIBS Qt5::OpenGL
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsInstrumentView MantidQtWidgetsPlotting MantidQtWidgetsMplCpp
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)
# Testing target We can currently only include the tests if we are building the framework. This is bceause we depend on
# a non-exported library of frameworks in the tests.
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
