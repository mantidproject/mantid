#
# Unit tests for MplCpp library
#
set (TEST_HEADERS
  PythonObjectTest.h
  MplFigureCanvasTest.h
  NDArray1DTest.h
)

# CXXTEST_EXTRA_HEADER_INCLUDE passes this file to the --include argument
set (CXXTEST_EXTRA_HEADER_INCLUDE GlobalInitialization.h)

include_directories (SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR}
                     ${GTEST_INCLUDE_DIR})
cxxtest_add_test (MantidQtWidgetsMplCppTest ${TEST_HEADERS} )
target_include_directories (MantidQtWidgetsMplCppTest PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../inc)
target_link_libraries (MantidQtWidgetsMplCppTest Qt4::QtGui MantidQtWidgetsMplCpp ${PYTHON_LIBRARIES})
add_dependencies( GUITests MantidQtWidgetsMplCppTest )
