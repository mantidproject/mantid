# Testing
set(
  TEST_FILES
  InstrumentWidgetDecoderTest.h
  InstrumentWidgetEncoderTest.h
  BaseCustomInstrumentPresenterTest.h
  PlotFitAnalysisPaneModelTest.h
  PlotFitAnalysisPanePresenterTest.h

  InstrumentWidget/InstrumentWidgetTest.h
)

set(
  MOCK_HEADER_DIRS
  InstrumentWidget
)

set(CXXTEST_EXTRA_HEADER_INCLUDE
    "${CMAKE_CURRENT_LIST_DIR}/InstrumentViewTestInitialization.h")

mtd_add_qt_tests(
  TARGET_NAME MantidQtWidgetsInstrumentWidgetTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS
    ${MOCK_HEADER_DIRS}

    ../inc/MantidQtWidgets/InstrumentView
    ../../../../Framework/DataObjects/inc
    ../../../../Framework/TestHelpers/inc
  TEST_HELPER_SRCS
    ../../../../Framework/TestHelpers/src/TearDownWorld.cpp
    ../../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
  LINK_LIBS
    Python::Python
    ${CORE_MANTIDLIBS}
    PythonInterfaceCore
    DataObjects
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    gmock
  QT5_LINK_LIBS Qt5::OpenGL
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsInstrumentView
    MantidQtWidgetsMplCpp
    MantidQtWidgetsPlotting
  PARENT_DEPENDENCIES GUITests
)
