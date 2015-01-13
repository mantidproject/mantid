if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  cxxtest_add_test ( MantidWidgetsTest ${TEST_FILES} )
  target_link_libraries( MantidWidgetsTest MantidWidgets )

  # Add to the 'UnitTests' group in VS
  set_property( TARGET MantidWidgetsTest PROPERTY FOLDER "UnitTests" )
endif ()

if ( PYUNITTEST_FOUND )
  pyunittest_add_test (${CMAKE_CURRENT_SOURCE_DIR} MantidWidgetsTest ${TEST_PY_FILES} )
endif ()
