# Testing
set(TEST_FILES ALFAnalysisModelTest.h ALFAnalysisPresenterTest.h ALFInstrumentModelTest.h ALFInstrumentPresenterTest.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesDirectTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS ../
  LINK_LIBS Mantid::API
            Mantid::DataObjects
            Mantid::Geometry
            Mantid::Kernel
            Mantid::PythonInterfaceCore
            Python::Python
            gmock
  MTD_QT_LINK_LIBS MantidScientificInterfacesDirect MantidQtWidgetsCommon MantidQtWidgetsInstrumentView
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtInterfacesDirectTestQt5)
