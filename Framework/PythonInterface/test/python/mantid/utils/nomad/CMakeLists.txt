# mantid.utils.nomad tests

set(TEST_PY_FILES diagnosticsTest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfaceUtils
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.utils.nomad ${TEST_PY_FILES})
