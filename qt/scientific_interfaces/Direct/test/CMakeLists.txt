# Testing
set(
  TEST_FILES
  ALFCustomInstrumentModelTest.h
  ALFCustomInstrumentPresenterTest.h
)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesDirectTest
  QT_VERSION 4
  SRC ${TEST_FILES}
  INCLUDE_DIRS
    ../../../../Framework/CurveFitting/inc
    ../../../../Framework/DataObjects/inc
    ../../../../Framework/TestHelpers/inc
    ../
  TEST_HELPER_SRCS
    ../../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
    #../../../../Framework/TestHelpers/src/IndirectFitDataCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/TearDownWorld.cpp
    #../IndirectDataValidationHelper.cpp
  LINK_LIBS
    CurveFitting
    DataObjects
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
  QT4_LINK_LIBS
    Qt4::QtOpenGL
    Qwt5
  MTD_QT_LINK_LIBS
    MantidScientificInterfacesDirect
    MantidQtWidgetsCommon
    MantidQtWidgetsInstrumentView
    MantidQtWidgetsPlotting
  PARENT_DEPENDENCIES
    GUITests)
