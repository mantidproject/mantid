# Tests for SANS

add_subdirectory(models)
add_subdirectory(presenter_tests)

set(TEST_PY_FILES
    add_runs_presenter_test.py
    beam_centre_model_test.py
    beam_centre_presenter_test.py
    concrete_run_tab_listener_test.py
    create_state_test.py
    diagnostics_page_model_test.py
    diagnostics_page_presenter_test.py
    gui_common_test.py
    gui_state_director_test.py
    masking_table_presenter_test.py
    model_common_test.py
    run_selector_presenter_test.py
    save_other_presenter_test.py
    settings_diagnostic_presenter_test.py
    state_gui_model_test.py
    summation_settings_model_test.py
    summation_settings_presenter_test.py
    table_model_test.py
    test_run_tab_presenter.py
    wavelength_interval_test.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

set(PYUNITTEST_QT_API pyqt5)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt5 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
