# Tests for ISIS Powder

set(TEST_PY_FILES
    # Calibration
    engineering_diffraction/tabs/calibration/test/test_model.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.EngineeringDiffraction
                    ${TEST_PY_FILES})