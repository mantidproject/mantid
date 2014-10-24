if ( PYUNITTEST_FOUND )
  pyunittest_add_test (${CMAKE_CURRENT_SOURCE_DIR} MantidWidgetsTest ${TEST_PY_FILES} )
endif ()

if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MantidWidgetsTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( MantidWidgetsTest MantidWidgets ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )

    # Add to the 'UnitTests' group in VS
    set_property( TARGET MantidWidgetsTest PROPERTY FOLDER "UnitTests" )
  endif ()

endif ()
