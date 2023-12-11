# mantid.utils.reflectometry tests

set(TEST_PY_FILES orso_helper_test.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfaceUtils
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.utils.reflectometry ${TEST_PY_FILES})
