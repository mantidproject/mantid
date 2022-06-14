# Testing
set(TEST_FILES
    ConvFitDataPresenterTest.h
    ConvFitModelTest.h
    ConvFunctionModelTest.h
    FqFitDataPresenterTest.h
    FqFitModelTest.h
    IDAFunctionParameterEstimationTest.h
    IndirectDataValidationHelperTest.h
    IndirectFitAnalysisTabTest.h
    IndirectFitDataPresenterTest.h
    IndirectFitDataTest.h
    IndirectFitDataModelTest.h
    IndirectFitOutputTest.h
    IndirectFitOutputOptionsModelTest.h
    IndirectFitOutputOptionsPresenterTest.h
    IndirectFitPlotModelTest.h
    IndirectFitPlotPresenterTest.h
    IndirectFitPropertyBrowserTest.h
    IndirectFittingModelTest.h
    IndirectMomentsModelTest.h
    IndirectPlotOptionsModelTest.h
    IndirectPlotOptionsPresenterTest.h
    IndirectSettingsModelTest.h
    IndirectSettingsPresenterTest.h
    IqtFitModelTest.h
)

set(CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/InterfacesIndirectTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesIndirectTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS ../../../../Framework/DataObjects/inc ../
  TEST_HELPER_SRCS ../IndirectDataValidationHelper.cpp
  LINK_LIBS ${CORE_MANTIDLIBS}
            Mantid::DataObjects
            gmock
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Mantid::PythonInterfaceCore
            ${PYTHON_LIBRARIES}
  QT5_LINK_LIBS Qt5::OpenGL Qt5::Concurrent
  MTD_QT_LINK_LIBS MantidScientificInterfacesIndirect MantidQtWidgetsCommon MantidQtWidgetsPlotting
                   MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtInterfacesIndirectTestQt5)
