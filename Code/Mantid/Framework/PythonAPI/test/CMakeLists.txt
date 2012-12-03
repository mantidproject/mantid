# cxxtest unit tests
if ( CXXTEST_FOUND )
  set ( TEST_FILES
    PythonFrameworkTests.h
  )

  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )
  include_directories ( ../../TestHelpers/inc )
  cxxtest_add_test ( PythonAPITest ${TEST_FILES} )
  target_link_libraries( PythonAPITest PythonAPI )
  
  # Specify implicit dependency, but don't link to it
  add_dependencies ( PythonAPITest UserAlgorithms )
  add_dependencies ( FrameworkTests PythonAPITest )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonAPITest PROPERTY FOLDER "UnitTests" )
endif ()

# Python unit tests
if (PYUNITTEST_FOUND)
  # Add test targets
  pyunittest_add_test_two ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAPI ${TEST_PY_FILES} )
endif ()

