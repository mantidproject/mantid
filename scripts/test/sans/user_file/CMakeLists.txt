# Tests for SANSState

set(TEST_PY_FILES user_file_parser_test.py user_file_reader_test.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonAlgorithms
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.SANS.user_file ${TEST_PY_FILES})

add_subdirectory(toml_parsers)
add_subdirectory(txt_parsers)
