if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories ( ../../MDEvents/inc ../../MDAlgorithms/inc ../../Nexus/inc ../../NexusCPP/inc )

  cxxtest_add_test ( PSISINQTest ${TEST_FILES} )
  target_link_libraries ( PSISINQTest SINQ  ${MANTIDLIBS} MDEvents )
  add_dependencies ( FrameworkTests PSISINQTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET PSISINQTest PROPERTY FOLDER "UnitTests" )
endif ()

