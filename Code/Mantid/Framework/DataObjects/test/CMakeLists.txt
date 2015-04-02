if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories ( ../../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../../TestHelpers/src/TearDownWorld.cpp
                        ../../TestHelpers/src/ComponentCreationHelper.cpp
                        ../../TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../../TestHelpers/src/NexusTestHelper.cpp
                        ../../TestHelpers/src/MDEventsTestHelper.cpp 
                        ../../TestHelpers/src/ScopedFileHelper.cpp
                        ../../TestHelpers/src/BoxControllerDummyIO.cpp
  )

  cxxtest_add_test ( DataObjectsTest ${TEST_FILES} )
  target_link_libraries( DataObjectsTest DataObjects ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES})
  # Specify implicit dependency, but don't link to it
  add_dependencies ( DataObjectsTest UserAlgorithms )
  add_dependencies ( FrameworkTests DataObjectsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET DataObjectsTest PROPERTY FOLDER "UnitTests" )
endif ()

