# Testing
set(
  TEST_FILES
  ALCBaselineModellingModelTest.h
  ALCBaselineModellingPresenterTest.h
  ALCDataLoadingPresenterTest.h
  ALCLatestFileFinderTest.h
  ALCPeakFittingModelTest.h
  ALCPeakFittingPresenterTest.h)

set(CXXTEST_EXTRA_HEADER_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/InterfacesMuonTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesMuonTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS
    ../../../../Framework/DataObjects/inc
    ../../../../Framework/TestHelpers/inc
    ../
  TEST_HELPER_SRCS
    ../../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/ScopedFileHelper.cpp
    ../../../../Framework/TestHelpers/src/TearDownWorld.cpp
  LINK_LIBS
    ${CORE_MANTIDLIBS}
    DataObjects
    gmock
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    PythonInterfaceCore
    ${PYTHON_LIBRARIES}
  QT5_LINK_LIBS
    Qt5::Test
    Qt5::OpenGL
  MTD_QT_LINK_LIBS
    MantidScientificInterfacesMuon
    MantidQtWidgetsCommon
    MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES
    GUITests)