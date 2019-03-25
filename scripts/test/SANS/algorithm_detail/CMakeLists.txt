# Tests for SANS

set(TEST_PY_FILES
    calculate_transmission_helper_test.py
    crop_helper_test.py
    merge_reductions_test.py
    q_resolution_calculator_test.py
    scale_helper_test.py
    strip_end_nans_test.py
    centre_finder_new_test.py
    batch_execution_test.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.SANS.algorithm_detail
                    ${TEST_PY_FILES})
