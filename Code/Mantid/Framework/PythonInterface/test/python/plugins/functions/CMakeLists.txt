##
## Tests for Fit functions
##

set ( TEST_PY_FILES
  Example1DFunctionTest.py
  ExamplePeakFunctionTest.py
  StretchedExpFTTest.py  # comment until scipy installed on the MAC
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonFunctions
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonFunctions ${TEST_PY_FILES} )
