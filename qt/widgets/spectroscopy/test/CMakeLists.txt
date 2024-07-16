set(TEST_FILES
    DataValidationHelperTest.h
    DataModelTest.h
    FitDataTest.h
    OutputPlotOptionsModelTest.h
    OutputPlotOptionsPresenterTest.h
    RunWidget/RunPresenterTest.h
    SettingsModelTest.h
    SettingsPresenterTest.h
    ValidationUtilsTest.h
    InterfaceUtilsTest.h
)

set(MOCK_HEADER_DIRS InstrumentWidget)

set(CXXTEST_EXTRA_HEADER_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/SpectroscopyTestInitialization.h")

mtd_add_qt_tests(
  TARGET_NAME MantidQtWidgetsSpectroscopyTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS ${MOCK_HEADER_DIRS} ../../../../Framework/DataObjects/inc
  TEST_HELPER_SRCS MockObjects.h
  LINK_LIBS Python::Python
            ${CORE_MANTIDLIBS}
            Mantid::PythonInterfaceCore
            Mantid::DataObjects
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            gmock
  MTD_QT_LINK_LIBS MantidQtWidgetsSpectroscopy
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtWidgetsSpectroscopyTestQt5)
