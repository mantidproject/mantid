add_subdirectory(common)
add_subdirectory(texture)

set(TEST_PY_FILES test_EnggUtils.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Engineering ${TEST_PY_FILES})
