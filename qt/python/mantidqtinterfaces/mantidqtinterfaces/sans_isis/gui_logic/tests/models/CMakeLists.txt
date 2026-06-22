set(TEST_PY_FILES test_file_loading.py test_settings_adjustment_model.py test_save_options.py test_run_tab_model.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

set(PYUNITTEST_QT_API pyqt${MANTID_QT_VERSION})
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt${MANTID_QT_VERSION} ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
