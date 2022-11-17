# Tests for DNS GUI

set(TEST_PY_FILES
    command_line/command_check_test.py
    data_structures/dns_binning_test.py
    data_structures/dns_elastic_powder_dataset_test.py
    data_structures/dns_plot_list_test.py
    data_structures/field_names_test.py
    options/elastic_powder_options_presenter_test.py
    options/elastic_powder_options_widget_test.py
    plot/elastic_powder_plot_model_test.py
    plot/elastic_powder_plot_presenter_test.py
    plot/elastic_powder_plot_widget_test.py
    scripts/md_powder_elastic_test.py
    script_generator/elastic_powder_script_generator_model_test.py
    script_generator/elastic_powder_script_generator_presenter_test.py
    script_generator/elastic_powder_script_generator_widget_test.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonAlgorithms
set(PYUNITTEST_QT_API pyqt5) # force to use qt5
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.DNSReductionQt5 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
