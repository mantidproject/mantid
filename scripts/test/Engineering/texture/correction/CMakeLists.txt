# Unit tests for correction engineering diffraction code

set(TEST_PY_FILES test_correction_model.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Engineering.texture.correction ${TEST_PY_FILES})
