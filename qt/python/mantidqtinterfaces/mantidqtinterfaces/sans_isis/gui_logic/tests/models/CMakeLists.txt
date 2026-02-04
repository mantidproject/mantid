set(TEST_PY_FILES test_file_loading.py test_row_entries.py test_settings_adjustment_model.py test_save_options.py
                  test_run_tab_model.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

set(PYUNITTEST_QT_API pyqt5)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt5 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
