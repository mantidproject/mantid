#
# Unit tests for MplCpp library
#
set (TEST_HEADERS
  MplFigureCanvasTest.h
  PythonObjectTest.h
  NDArray1DTest.h
)

# CXXTEST_EXTRA_HEADER_INCLUDE passes this file to the --include argument
set (CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/GlobalInitialization.h)

include_directories (SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR}
                     ${GTEST_INCLUDE_DIR})
cxxtest_add_test (MantidQtWidgetsMplCppTest ${TEST_HEADERS} )
target_include_directories (MantidQtWidgetsMplCppTest PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../inc)
target_link_libraries (MantidQtWidgetsMplCppTest Qt4::QtGui MantidQtWidgetsMplCpp ${PYTHON_LIBRARIES})
add_dependencies( GUITests MantidQtWidgetsMplCppTest )
# Add to the 'UnitTests' group in VS
set_property( TARGET MantidQtWidgetsMplCppTest PROPERTY FOLDER "UnitTests" )
