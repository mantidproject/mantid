if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  # This is required to pick up ComponentCreationHelper, which is the only TestHelpers class that may be 
  # used by Geometry tests (as it does not depend on any higher package than geometry)
  include_directories( ../../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../../TestHelpers/src/ComponentCreationHelper.cpp
                        ../../TestHelpers/src/NexusTestHelper.cpp
                        ../../TestHelpers/src/ScopedFileHelper.cpp
  )

  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( GeometryTest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( GeometryTest Geometry ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )
  else ()
    cxxtest_add_test ( GeometryTest ${TEST_FILES} )
    target_link_libraries( GeometryTest Geometry )
  endif()
  
  add_dependencies ( FrameworkTests GeometryTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET GeometryTest PROPERTY FOLDER "UnitTests" )
endif ()

