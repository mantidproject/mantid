if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories ( ../../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../../TestHelpers/src/ComponentCreationHelper.cpp
                        ../../TestHelpers/src/WorkspaceCreationHelper.cpp
                         ../../TestHelpers/src/ScopedFileHelper.cpp
                         ../../TestHelpers/src/StartFrameworkManager.cpp
      )

  cxxtest_add_test ( RemoteTest ${TEST_FILES} )
  target_link_libraries( RemoteTest Remote )
  add_dependencies ( FrameworkTests RemoteTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET RemoteTest PROPERTY FOLDER "UnitTests" )
endif ()

