#
## Tests for SANSState
##

set ( TEST_PY_FILES
   MaskParserTest.py
   SANSStateTest.py
   SANSStateDataTest.py
   SANSStateDataBuilderTest.py
   SANSStateReductionTest.py
   SANSStateReductionBuilderTest.py
   SANSStateSliceEventTest.py
   SANSStateSliceEventBuilderTest.py
   SANSStateMoveTest.py
   SANSStateMaskTest.py
   SANSStateMaskBuilderTest.py
   SANSStateMoveBuilderTest.py
  )

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithmsSANS ${TEST_PY_FILES} )
