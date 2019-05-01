# mantid._plugin tests

set(TEST_PY_FILES
    ProductFunctionTest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfacePlugins
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python._plugins
                    ${TEST_PY_FILES})
