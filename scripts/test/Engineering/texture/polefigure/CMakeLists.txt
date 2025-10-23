# Unit tests for polefigure engineering diffraction code

set(TEST_PY_FILES test_polefigure_model.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Engineering.texture.polefigure ${TEST_PY_FILES})
