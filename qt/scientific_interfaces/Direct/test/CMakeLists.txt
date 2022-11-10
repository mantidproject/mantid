# Testing
set(TEST_FILES ALFAnalysisModelTest.h ALFAnalysisPresenterTest.h ALFInstrumentModelTest.h ALFInstrumentPresenterTest.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesDirectTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS ../../../../Framework/CurveFitting/inc ../../../../Framework/DataObjects/inc ../
  LINK_LIBS Mantid::DataObjects
            ${CORE_MANTIDLIBS}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Mantid::PythonInterfaceCore
            Python::Python
            gmock
  MTD_QT_LINK_LIBS MantidScientificInterfacesDirect MantidQtWidgetsCommon MantidQtWidgetsInstrumentView
                   MantidQtWidgetsPlotting MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtInterfacesDirectTestQt5)
