set ( TEST_PY_FILES test/ReducerTest.py )

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( PythonReductionTest.py ${TEST_PY_FILES} )
  add_dependencies ( PythonReductionTest.py PythonAPI )
  add_dependencies ( FrameworkTests PythonReductionTest.py )
endif ()