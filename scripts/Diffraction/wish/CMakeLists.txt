# Tests for common single crystal reduction

set(TEST_PY_FILES test/test_wish_powder.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Diffraction.wish ${TEST_PY_FILES})
