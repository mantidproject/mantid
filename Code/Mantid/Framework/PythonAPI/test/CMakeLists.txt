if ( CXXTEST_FOUND )
  include_directories ( ../../TestHelpers/inc )
  cxxtest_add_test ( PythonAPITest ${TEST_FILES} )
  target_link_libraries( PythonAPITest PythonAPI )
  
  # Specify implicit dependency, but don't link to it
  add_dependencies ( PythonAPITest UserAlgorithms )
  add_dependencies ( FrameworkTests PythonAPITest )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonAPITest PROPERTY FOLDER "UnitTests" )
endif ()

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( VanillaPythonTest.py ${TEST_PY_FILES} "" )
  add_dependencies ( VanillaPythonTest.py PythonAPI Algorithms )
  add_dependencies ( FrameworkTests VanillaPythonTest.py )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET VanillaPythonTest.py PROPERTY FOLDER "UnitTests" )
endif ()

