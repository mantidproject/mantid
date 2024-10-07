set(TEST_PY_FILES
    add_runs_presenter_test.py
    beam_centre_presenter_test.py
    diagnostics_page_presenter_test.py
    masking_table_presenter_test.py
    presenter_common_test.py
    run_selector_presenter_test.py
    save_other_presenter_test.py
    settings_adjustment_presenter_test.py
    settings_diagnostic_presenter_test.py
    summation_settings_presenter_test.py
    test_run_tab_presenter.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

set(PYUNITTEST_QT_API pyqt5)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt5 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
