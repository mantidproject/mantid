if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories ( ../../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS TestDataListener.cpp
                        TestGroupDataListener.cpp
                        ../../TestHelpers/src/TearDownWorld.cpp
                        ../../TestHelpers/src/ComponentCreationHelper.cpp
                        ../../TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../../TestHelpers/src/StartFrameworkManager.cpp
      )

  cxxtest_add_test ( LiveDataTest ${TEST_FILES} )
  target_link_libraries( LiveDataTest LiveData )
  add_dependencies ( LiveDataTest DataHandling Algorithms MDAlgorithms )
  add_dependencies ( FrameworkTests LiveDataTest )
  # Test data
  add_dependencies ( LiveDataTest StandardTestData )

  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET LiveDataTest PROPERTY FOLDER "UnitTests" )
endif ()

