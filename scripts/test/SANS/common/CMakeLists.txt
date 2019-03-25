# Tests for SANS

set(TEST_PY_FILES
    enums_test.py
    file_information_test.py
    log_tagger_test.py
    general_functions_test.py
    xml_parsing_test.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANS ${TEST_PY_FILES})
