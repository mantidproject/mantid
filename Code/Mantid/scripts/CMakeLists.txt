set ( TEST_PY_FILES test/ReducerTest.py )

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( PythonReductionTest.py ${TEST_PY_FILES} )
  add_dependencies ( PythonReductionTest.py PythonAPI )
  add_dependencies ( FrameworkTests PythonReductionTest.py )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonReductionTest.py PROPERTY FOLDER "UnitTests" )
endif ()

# Ensure we don't get stale pyc files around
clean_orphaned_pyc_files ( ${CMAKE_CURRENT_SOURCE_DIR} )
