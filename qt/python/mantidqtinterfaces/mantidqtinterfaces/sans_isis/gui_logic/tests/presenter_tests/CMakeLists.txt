set(TEST_PY_FILES presenter_common_test.py settings_adjustment_presenter_test.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

set(PYUNITTEST_QT_API pyqt${MANTID_QT_VERSION})
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt${MANTID_QT_VERSION} ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
