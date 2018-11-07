###########################################################################
# Testing
###########################################################################
set ( TEST_FILES
  IndirectFitDataTest.h
  IndirectFitOutputTest.h
  IndirectFitPlotModelTest.h
  IndirectFittingModelTest.h
)

mtd_add_qt_tests (TARGET_NAME MantidQtInterfacesIndirectTest
  QT_VERSION 4
  SRC ${TEST_FILES}
  INCLUDE_DIRS
    ../../../../Framework/CurveFitting/inc
    ../../../../Framework/DataObjects/inc
    ../../../../Framework/TestHelpers/inc
    ../
  TEST_HELPER_SRCS
    ../../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/IndirectFitDataCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
    ../../../../Framework/TestHelpers/src/TearDownWorld.cpp
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    CurveFitting
    DataObjects
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
  QT4_LINK_LIBS
    Qwt5
  MTD_QT_LINK_LIBS
    MantidScientificInterfacesIndirect
  PARENT_DEPENDENCIES
    GUITests
)
