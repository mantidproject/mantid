set(TEST_PY_FILES toml_parser_impl_base_test.py wavelength_parser_test.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.SANS.user_file.txt_parsers ${TEST_PY_FILES})
