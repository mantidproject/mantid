# Tests for SANS

set(TEST_PY_FILES
    batch_execution_test.py
    calculate_sans_transmission_test.py
    calculate_transmission_helper_test.py
    centre_finder_new_test.py
    create_sans_adjustment_workspaces_test.py
    create_sans_wavelength_pixel_adjustment_test.py
    convert_to_q_test.py
    crop_helper_test.py
    mask_workspace_test.py
    mask_sans_workspace_test.py
    merge_reductions_test.py
    move_sans_instrument_component_test.py
    move_workspaces_test.py
    normalize_to_sans_monitor_test.py
    sans_slice_event_test.py
    scale_sans_workspace_test.py
    strip_end_nans_test.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.SANS.algorithm_detail ${TEST_PY_FILES})
