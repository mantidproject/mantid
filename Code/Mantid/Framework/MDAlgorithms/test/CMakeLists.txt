if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories( ../../TestHelpers/inc ../../DataHandling/inc ../../CurveFitting/inc)	
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../../TestHelpers/src/TearDownWorld.cpp
                        ../../TestHelpers/src/ComponentCreationHelper.cpp
                        ../../TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../../TestHelpers/src/MDAlgorithmsTestHelper.cpp
                        ../../TestHelpers/src/MDEventsTestHelper.cpp
                        ../../TestHelpers/src/BinaryOperationMDTestHelper.cpp )
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( MDAlgorithmsTest MDAlgorithms DataHandling CurveFitting Nexus ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} ${NEXUS_LIBRARIES} )
  else ()
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} )
    target_link_libraries( MDAlgorithmsTest MDAlgorithms Nexus ${NEXUS_LIBRARIES} )
  endif ()
  add_dependencies ( MDAlgorithmsTest DataHandling Algorithms CurveFitting )
  add_dependencies ( FrameworkTests MDAlgorithmsTest )
  # Test data
  add_dependencies ( MDAlgorithmsTest StandardTestData )

  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDAlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()

