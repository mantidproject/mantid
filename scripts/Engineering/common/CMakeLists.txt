# Tests for Engineering common code

set(TEST_PY_FILES test/test_calibration_info.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Engineering.common ${TEST_PY_FILES})
