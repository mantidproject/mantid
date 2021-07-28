# Testing
set(
  TEST_FILES
  ConvFitDataPresenterTest.h
  ConvFitModelTest.h
  ConvFunctionModelTest.h
  FqFitDataPresenterTest.h
  FqFitModelTest.h
  IDAFunctionParameterEstimationTest.h
  IndirectDataValidationHelperTest.h
  IndirectFitDataTablePresenterTest.h
  IndirectFitAnalysisTabTest.h
  IndirectFitDataPresenterTest.h
  IndirectFitDataTest.h
  IndirectFitDataTableModelTest.h
  IndirectFitOutputModelTest.h
  IndirectFitOutputOptionsModelTest.h
  IndirectFitOutputOptionsPresenterTest.h
  IndirectFitPlotModelTest.h
  IndirectFitPlotPresenterTest.h
  IndirectFitPropertyBrowserTest.h
  IndirectFittingModelTest.h
  IndirectPlotOptionsModelTest.h
  IndirectPlotOptionsPresenterTest.h
  IndirectSettingsModelTest.h
  IndirectSettingsPresenterTest.h
  IqtFitModelTest.h
)

set(CXXTEST_EXTRA_HEADER_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/InterfacesIndirectTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesIndirectTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS
    ../../../../Framework/DataObjects/inc
    ../../../../Framework/TestHelpers/inc
    ../
  TEST_HELPER_SRCS
    ../../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/IndirectFitDataCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/TearDownWorld.cpp
    ../IndirectDataValidationHelper.cpp
  LINK_LIBS
    ${CORE_MANTIDLIBS}
    Mantid::DataObjects
    gmock
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    Mantid::PythonInterfaceCore
    ${PYTHON_LIBRARIES}
  QT5_LINK_LIBS
    Qt5::OpenGL
    Qt5::Concurrent
  MTD_QT_LINK_LIBS
    MantidScientificInterfacesIndirect
    MantidQtWidgetsCommon
    MantidQtWidgetsPlotting
    MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES
    GUITests)

