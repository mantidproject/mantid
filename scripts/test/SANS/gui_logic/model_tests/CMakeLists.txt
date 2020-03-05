set(TEST_PY_FILES
    settings_adjustment_model_test.py
    )

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

set(PYUNITTEST_QT_API pyqt) # force to use qt4
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt4 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)

set(PYUNITTEST_QT_API pyqt5)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt5 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)