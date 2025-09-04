# Tests for Engineering scripts/common code

set(TEST_PY_FILES test/test_EnggUtils.py test/test_pawley_utils.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Engineering ${TEST_PY_FILES})
