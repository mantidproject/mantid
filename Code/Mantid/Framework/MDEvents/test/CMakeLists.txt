if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories ( ../../TestHelpers/inc ../../DataHandling/inc )
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../../TestHelpers/src/TearDownWorld.cpp
                        ../../TestHelpers/src/ComponentCreationHelper.cpp
                        ../../TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../../TestHelpers/src/MDEventsTestHelper.cpp 
                        ../../TestHelpers/src/ScopedFileHelper.cpp
                        ../../TestHelpers/src/BoxControllerDummyIO.cpp
                        )
  
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDEventsTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( MDEventsTest MDEvents ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} DataHandling )
  else ()
    cxxtest_add_test ( MDEventsTest ${TEST_FILES} )
    target_link_libraries( MDEventsTest MDEvents DataHandling)
  endif ()
  
  add_dependencies ( FrameworkTests MDEventsTest )
  # Test data
  add_dependencies ( MDEventsTest StandardTestData )
  
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDEventsTest PROPERTY FOLDER "UnitTests" )
  
endif ()

