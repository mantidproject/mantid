# Unit tests for vulcan/calibration

set(TEST_PY_FILES
    test_all.py
    test_calibration.py
    )

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.vulcan.calibration
                    ${TEST_PY_FILES})
