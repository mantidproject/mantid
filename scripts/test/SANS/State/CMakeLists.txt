#
## Tests for SANSState
##

set ( TEST_PY_FILES
   SANSStateTest.py
   SANSStateDataTest.py
   SANSStateReductionTest.py
   SANSStateMoveBuilderTest.py
   SANSStateMoveTest.py
  )

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithmsSANS ${TEST_PY_FILES} )
