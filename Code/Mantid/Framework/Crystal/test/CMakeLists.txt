if ( CXXTEST_FOUND )
  include_directories ( ../../DataHandling/inc ../../MDEvents/inc ../../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../../TestHelpers/src/ComponentCreationHelper.cpp 
                        ../../TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../../TestHelpers/src/MDEventsTestHelper.cpp )
  cxxtest_add_test ( CrystalTest ${TEST_FILES} )
  target_link_libraries ( CrystalTest Crystal DataHandling MDEvents )
  add_dependencies ( FrameworkTests CrystalTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET CrystalTest PROPERTY FOLDER "UnitTests" )
endif ()

