add_subdirectory(async_workers)

set(TEST_PY_FILES test_beam_centre_async.py test_sans_run_tab_async.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

set(PYUNITTEST_QT_API pyqt5)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt5 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
