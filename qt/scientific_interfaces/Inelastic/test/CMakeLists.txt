# Testing
set(TEST_FILES
    ConvFitModelTest.h
    ConvFunctionModelTest.h
    IDAFunctionParameterEstimationTest.h
    IqtFitModelTest.h
    FqFitModelTest.h
    FqFitDataPresenterTest.h
    ConvFitDataPresenterTest.h
    IndirectDataAnalysisTabTest.h
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
    InelasticDataManipulationElwinTabModelTest.h
    InelasticDataManipulationIqtTabModelTest.h
    InelasticDataManipulationMomentsTabModelTest.h
    InelasticDataManipulationSqwTabModelTest.h
    InelasticDataManipulationSymmetriseTabModelTest.h
)

set(TEST_HELPERS ../../Indirect/IndirectDataValidationHelper.cpp DataAnalysisMockObjects.h)

set(CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/InterfacesInelasticTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesInelasticTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS ../../../../Framework/DataObjects/inc ../
  TEST_HELPER_SRCS ${TEST_HELPERS}
  LINK_LIBS ${CORE_MANTIDLIBS}
            Mantid::DataObjects
            gmock
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Mantid::PythonInterfaceCore
            ${PYTHON_LIBRARIES}
  QT5_LINK_LIBS Qt5::OpenGL Qt5::Concurrent
  MTD_QT_LINK_LIBS MantidScientificInterfacesInelastic MantidQtWidgetsCommon MantidQtWidgetsPlotting
                   MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtInterfacesInelasticTestQt5)
