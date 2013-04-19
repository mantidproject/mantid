##
## Tests for Fit functions
##

set ( TEST_PY_FILES
  Example1DFunctionTest.py
  ExamplePeakFunctionTest.py
)

# Prefix for test name=PythonFunctions
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonFunctions ${TEST_PY_FILES} )
