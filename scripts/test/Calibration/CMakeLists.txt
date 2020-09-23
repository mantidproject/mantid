# Unit tests for Calibration

set(TEST_PY_FILES
    test_ideal_tube.py
    )

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Calibration
                    ${TEST_PY_FILES})
