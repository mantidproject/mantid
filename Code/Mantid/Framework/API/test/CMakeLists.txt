if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories ( ../../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../../TestHelpers/src/TearDownWorld.cpp
                        ../../TestHelpers/src/ComponentCreationHelper.cpp
                        ../../TestHelpers/src/BoxControllerDummyIO.cpp
                        ../../TestHelpers/src/NexusTestHelper.cpp
      )
  
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( APITest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( APITest API ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES}  )
  else ()
    cxxtest_add_test ( APITest ${TEST_FILES} )
    target_link_libraries( APITest API  )
  endif()
  add_dependencies ( FrameworkTests APITest )
  # Test data
  add_dependencies ( APITest StandardTestData )

  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET APITest PROPERTY FOLDER "UnitTests" )
endif ()
