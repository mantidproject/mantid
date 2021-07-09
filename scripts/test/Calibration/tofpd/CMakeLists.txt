# Unit tests for Calibration.tofpd

set(TEST_PY_FILES
    test_diagnostics.py
    test_group_calibration.py
    )

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Calibration.tofpd
                    ${TEST_PY_FILES})
