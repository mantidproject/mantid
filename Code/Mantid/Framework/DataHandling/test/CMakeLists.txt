if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories ( ../../TestHelpers/inc ../../Algorithms/inc )
  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../../TestHelpers/src/TearDownWorld.cpp
                        ../../TestHelpers/src/ComponentCreationHelper.cpp
                        ../../TestHelpers/src/ScopedFileHelper.cpp
                        ../../TestHelpers/src/StartFrameworkManager.cpp
                        ../../TestHelpers/src/WorkspaceCreationHelper.cpp
      )

  cxxtest_add_test ( DataHandlingTest ${TEST_FILES} )
  target_link_libraries( DataHandlingTest DataHandling Algorithms )
  add_dependencies ( DataHandlingTest Algorithms MDAlgorithms )
  add_dependencies ( FrameworkTests DataHandlingTest )
  # Test data
  add_dependencies ( DataHandlingTest StandardTestData )

  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET DataHandlingTest PROPERTY FOLDER "UnitTests" )
endif ()

