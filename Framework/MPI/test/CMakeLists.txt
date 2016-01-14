if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR}
    ../../API/inc )

  cxxtest_add_test ( MPITest ${TEST_FILES} )
  target_link_libraries ( MPITest MPI API )
  add_dependencies ( FrameworkTests MPITest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MPITest PROPERTY FOLDER "UnitTests" )
endif ()
