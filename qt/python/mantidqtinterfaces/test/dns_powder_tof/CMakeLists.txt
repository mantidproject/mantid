# Tests for DNS GUI

set(TEST_PY_FILES
    main_presenter_test.py
    main_widget_test.py
    dns_modus_test.py
    parameter_abo_test.py
    data_structures/dns_error_test.py
    data_structures/dns_file_test.py
    data_structures/dns_observer_test.py
    data_structures/dns_obs_model_test.py
    data_structures/dns_tof_powder_dataset_test.py
    data_structures/dns_treeitem_test.py
    data_structures/dns_treemodel_test.py
    data_structures/dns_view_test.py
    data_structures/dns_widget_test.py
    data_structures/object_dict_test.py
    file_selector/file_selector_model_test.py
    file_selector/file_selector_presenter_test.py
    file_selector/file_selector_widget_test.py
    helpers/converters_test.py
    helpers/file_processing_test.py
    helpers/list_range_converters_test.py
    options/common_options_model_test.py
    options/common_options_presenter_test.py
    options/tof_powder_options_model_test.py
    options/tof_powder_options_presenter_test.py
    options/tof_powder_options_widget_test.py
    paths/path_model_test.py
    paths/path_presenter_test.py
    paths/path_widget_test.py
    plot/tof_powder_plot_model_test.py
    plot/tof_powder_plot_presenter_test.py
    plot/tof_powder_plot_widget_test.py
    scripts/md_powder_tof_test.py
    script_generator/common_script_generator_model_test.py
    script_generator/common_script_generator_presenter_test.py
    script_generator/tof_powder_script_generator_model_test.py
    script_generator/tof_powder_script_generator_presenter_test.py
    script_generator/tof_powder_script_generator_widget_test.py
    xml_dump/xml_dump_model_test.py
    xml_dump/xml_dump_presenter_test.py
    xml_dump/xml_dump_widget_test.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonAlgorithms
set(PYUNITTEST_QT_API pyqt5) # force to use qt5
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.DNSReductionQt5 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
