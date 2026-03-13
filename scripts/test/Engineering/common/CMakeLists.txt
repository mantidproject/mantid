# Unit tests for common engineering diffraction code

set(TEST_PY_FILES test_calibration_info.py test_texture_sample_viewer.py test_instrument_config.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Engineering.common ${TEST_PY_FILES})
