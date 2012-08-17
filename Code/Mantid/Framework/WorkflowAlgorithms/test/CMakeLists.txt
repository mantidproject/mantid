if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  # Make the rest of the tests
  include_directories ( ../../DataHandling/inc )
  cxxtest_add_test ( WorkflowAlgorithmsTest ${TEST_FILES} )
  target_link_libraries ( WorkflowAlgorithmsTest WorkflowAlgorithms Algorithms DataHandling )
  add_dependencies ( FrameworkTests WorkflowAlgorithmsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET WorkflowAlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( PythonWorkflowAlgorithmsTest.py ${TEST_PY_FILES} )
  add_dependencies ( PythonWorkflowAlgorithmsTest.py PythonAPI )
  add_dependencies ( FrameworkTests PythonWorkflowAlgorithmsTest.py )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonWorkflowAlgorithmsTest.py PROPERTY FOLDER "UnitTests" )
endif ()

