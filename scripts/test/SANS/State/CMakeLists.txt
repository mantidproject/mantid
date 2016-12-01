#
## Tests for SANSState
##

set ( TEST_PY_FILES
   SANSStateAdjustmentTest.py
   SANSStateAdjustmentBuilderTest.py
   SANSStateBaseTest.py
   SANSStateCalculateTransmissionTest.py
   SANSStateCalculateTransmissionBuilderTest.py
   SANSStateConvertToQTest.py
   SANSStateConvertToQBuilderTest.py
   SANSStateDataTest.py
   SANSStateDataBuilderTest.py
   SANSStateFunctionsTest.py
   SANSStateScaleTest.py
   SANSStateScaleBuilderTest.py
   SANSStateMaskTest.py
   SANSStateMaskBuilderTest.py
   SANSStateMoveTest.py
   SANSStateMoveBuilderTest.py
   SANSStateNormalizeToMonitorTest.py
   SANSStateNormalizeToMonitorBuilderTest.py
   SANSStateReductionTest.py
   SANSStateReductionBuilderTest.py
   SANSStateSliceEventTest.py
   SANSStateSliceEventBuilderTest.py
   SANSStateSaveTest.py
   SANSStateTest.py
   SANSStateSaveBuilderTest.py
   SANSStateWavelengthTest.py
   SANSStateWavelengthBuilderTest.py
   SANSStateWavelengthAndPixelAdjustmentTest.py
   SANSStateWavelengthAndPixelAdjustmentBuilderTest.py)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithmsSANS ${TEST_PY_FILES} )
