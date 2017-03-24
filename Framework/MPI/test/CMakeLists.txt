if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} ../../TestHelpers/inc)

  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS EnvironmentSetup.cpp )

  cxxtest_add_test ( MPITest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries( MPITest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} ${MANTIDLIBS}
    MPI
    ${Boost_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES} )

  add_dependencies ( FrameworkTests MPITest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MPITest PROPERTY FOLDER "UnitTests" )
endif ()
