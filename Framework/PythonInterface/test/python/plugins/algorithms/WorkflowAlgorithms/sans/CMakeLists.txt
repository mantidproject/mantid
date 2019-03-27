# Tests for ISIS SANS

set(TEST_PY_FILES
    SANSCalculateTransmissionTest.py
    SANSConvertToQTest.py
    SANSConvertToWavelengthTest.py
    SANSCreateAdjustmentWorkspacesTest.py
    SANSCreateWavelengthAndPixelAdjustmentTest.py
    SANSNormalizeToMonitorTest.py
    SANSScaleTest.py
    SANSSliceEventTest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonAlgorithms_SANS
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithms
                    ${TEST_PY_FILES})
