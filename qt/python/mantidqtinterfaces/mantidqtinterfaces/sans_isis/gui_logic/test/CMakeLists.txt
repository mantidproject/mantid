# Tests for SANS

add_subdirectory(model_tests)
add_subdirectory(presenter_tests)

set(TEST_PY_FILES concrete_run_tab_listener_test.py create_state_test.py gui_common_test.py gui_state_director_test.py
                  model_common_test.py wavelength_interval_test.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

set(PYUNITTEST_QT_API pyqt5)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt5 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
