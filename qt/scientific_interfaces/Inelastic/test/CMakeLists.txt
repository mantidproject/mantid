# Testing
set(TEST_FILES
    Analysis/ConvFitModelTest.h
    Analysis/ConvFitDataPresenterTest.h
    Analysis/ConvFunctionTemplateModelTest.h
    Analysis/FqFitDataPresenterTest.h
    Analysis/FqFitModelTest.h
    Analysis/IDAFunctionParameterEstimationTest.h
    Analysis/DataAnalysisTabTest.h
    Analysis/FitDataPresenterTest.h
    Analysis/FitDataTest.h
    Analysis/FitDataModelTest.h
    Analysis/FitOutputTest.h
    Analysis/FitOutputOptionsModelTest.h
    Analysis/FitOutputOptionsPresenterTest.h
    Analysis/FitPlotModelTest.h
    Analysis/FitPlotPresenterTest.h
    Analysis/InelasticFitPropertyBrowserTest.h
    Analysis/FittingModelTest.h
    Analysis/IqtFitModelTest.h
    Common/IndirectDataValidationHelperTest.h
    Common/OutputPlotOptionsModelTest.h
    Common/OutputPlotOptionsPresenterTest.h
    Common/SettingsModelTest.h
    Common/SettingsPresenterTest.h
    Common/WorkspaceUtilsTest.h
    Common/InterfaceUtilsTest.h
    Common/ValidationUtilsTest.h
    Manipulation/InelasticDataManipulationElwinTabModelTest.h
    Manipulation/InelasticDataManipulationIqtTabModelTest.h
    Manipulation/InelasticDataManipulationMomentsTabModelTest.h
    Manipulation/InelasticDataManipulationSqwTabModelTest.h
    Manipulation/InelasticDataManipulationSymmetriseTabModelTest.h
)

set(TEST_HELPERS Analysis/MockObjects.h Common/MockObjects.h)

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
