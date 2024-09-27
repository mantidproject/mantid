# Tests for SANS

set(TEST_PY_FILES batch_csv_file_parser_test.py command_interface_state_director_test.py isis_command_interface_test.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonAlgorithms
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.SANS.command_interface ${TEST_PY_FILES})
