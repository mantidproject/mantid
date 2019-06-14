# Tests for SANS

set(TEST_PY_FILES
    gui_state_director_test.py
    gui_common_test.py
    main_presenter_test.py
    masking_table_presenter_test.py
    property_manager_service_test.py
    run_tab_presenter_test.py
    sans_data_processor_gui_algorithm_test.py
    state_gui_model_test.py
    settings_diagnostic_presenter_test.py
    table_model_test.py
    run_selector_presenter_test.py
    summation_settings_presenter_test.py
    summation_settings_model_test.py
    add_runs_presenter_test.py
    beam_centre_presenter_test.py
    beam_centre_model_test.py
    diagnostics_page_presenter_test.py
    diagnostics_page_model_test.py
    create_state_test.py
    batch_process_runner_test.py
    save_other_presenter_test.py
    create_file_information_test.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

set(PYUNITTEST_QT_API pyqt) # force to use qt4
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt4 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)

set(PYUNITTEST_QT_API pyqt5)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonSANSQt5 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
