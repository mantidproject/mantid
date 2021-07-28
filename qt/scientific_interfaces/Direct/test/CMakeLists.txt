# Testing
set(
  TEST_FILES
  ALFCustomInstrumentModelTest.h
  ALFCustomInstrumentPresenterTest.h
)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesDirectTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS
    ../../../../Framework/CurveFitting/inc
    ../../../../Framework/DataObjects/inc
    ../../../../Framework/TestHelpers/inc
    ../
  TEST_HELPER_SRCS
    ../../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/TearDownWorld.cpp
  LINK_LIBS
    Mantid::DataObjects
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    Python::Python
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    gmock
  QT5_LINK_LIBS Qt5::OpenGL
  MTD_QT_LINK_LIBS
    MantidScientificInterfacesDirect
    MantidQtWidgetsCommon
    MantidQtWidgetsInstrumentView
    MantidQtWidgetsPlotting
    MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES
    GUITests)
