#
# Unit tests for MplCpp library
#
set ( TEST_HEADERS
  ArtistTest.h
  AxesTest.h
  FigureCanvasQtTest.h
  Line2DTest.h
  SipTest.h
)

# CXXTEST_EXTRA_HEADER_INCLUDE passes this file to the --include argument
set (CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/MplCppTestInitialization.h)
cxxtest_add_test ( MplCppQt5Test ${TEST_HEADERS} )

target_include_directories ( MplCppQt5Test SYSTEM PRIVATE
                             ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR}
                             ${GTEST_INCLUDE_DIR} )
target_include_directories (MplCppQt5Test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../inc
)
target_link_libraries ( MplCppQt5Test
  MplCppQt5 Qt5::Gui Qt5::Widgets PythonInterfaceCore ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} )

add_dependencies( GUITests MplCppQt5Test )

# Add to the 'UnitTests' group in VS
set_property( TARGET MplCppQt5Test PROPERTY FOLDER "UnitTests" )
