#
# Unit tests for MplCpp library
#
set ( TEST_FILES
  ArtistTest.h
  AxesTest.h
  ColorsTest.h
  ColormapTest.h
  FigureCanvasQtTest.h
  Line2DTest.h
  ScalarMappableTest.h
  SipTest.h
)

set (CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/MplCppTestInitialization.h)

mtd_add_qt_test_executable (TARGET_NAME MplCppTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  LINK_LIBS
    PythonInterfaceCore
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
  MTD_QT_LINK_LIBS
    MplCpp
  PARENT_DEPENDENCIES
    GUITests
)
