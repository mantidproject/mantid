# mantid._testing tests

set(TEST_PY_FILES AssertWorkspacesTest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfaceTesting
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python._testing ${TEST_PY_FILES})
