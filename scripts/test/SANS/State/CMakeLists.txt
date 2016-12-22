#
## Tests for SANSState
##

set ( TEST_PY_FILES
   SANSStateTest.py
   SANSStateFunctionsTest.py
   SANSStateDataTest.py
   SANSStateDataBuilderTest.py
   SANSStateReductionTest.py
   SANSStateReductionBuilderTest.py
   SANSStateSliceEventTest.py
   SANSStateSliceEventBuilderTest.py
   SANSStateMaskTest.py
   SANSStateMaskBuilderTest.py
   SANSStateMoveTest.py
   SANSStateMoveBuilderTest.py
   SANSStateSaveTest.py
   SANSStateSaveBuilderTest.py
   SANSStateWavelengthTest.py
   SANSStateWavelengthBuilderTest.py
  )

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithmsSANS ${TEST_PY_FILES} )
