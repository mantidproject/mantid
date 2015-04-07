if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  cxxtest_add_test ( RemoteJobManagersTest ${TEST_FILES} )
  target_link_libraries ( RemoteJobManagersTest RemoteJobManagers ${GMOCK_LIBRARIES} )
  add_dependencies ( FrameworkTests RemoteJobManagersTest )
  # Test data. Not using any for now. Remember to uncomment if data is added for these remote job managers
  # add_dependencies ( RemoteJobManagersTest StandardTestData )

  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET RemoteJobManagersTest PROPERTY FOLDER "UnitTests" )

endif ()
