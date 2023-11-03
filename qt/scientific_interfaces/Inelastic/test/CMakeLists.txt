# Testing
set(TEST_FILES IqtFitModelTest.h FqFitModelTest.h FqFitDataPresenterTest.h ConvFitDataPresenterTest.h
               IndirectDataAnalysisTabTest.h
)

set(CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/InterfacesInelasticTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesInelasticTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS ../../../../Framework/DataObjects/inc ../
  TEST_HELPER_SRCS ../../Indirect/IndirectDataValidationHelper.cpp
  LINK_LIBS ${CORE_MANTIDLIBS}
            Mantid::DataObjects
            gmock
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Mantid::PythonInterfaceCore
            ${PYTHON_LIBRARIES}
  QT5_LINK_LIBS Qt5::OpenGL Qt5::Concurrent
  MTD_QT_LINK_LIBS MantidScientificInterfacesIndirect MantidScientificInterfacesInelastic MantidQtWidgetsCommon
                   MantidQtWidgetsPlotting MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtInterfacesInelasticTestQt5)
