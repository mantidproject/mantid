# Tests for DNS GUI

set(TEST_PY_FILES
    data_structures/dns_single_crystal_map_test.py
    plot/elastic_single_crystal_helpers_test.py
    plot/elastic_single_crystal_plot_model_test.py
    plot/elastic_single_crystal_plot_plot_test.py
    plot/elastic_single_crystal_plot_presenter_test.py
    plot/elastic_single_crystal_plot_widget_test.py
    plot/grid_locator_test.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonAlgorithms
set(PYUNITTEST_QT_API pyqt5) # force to use qt5
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.DNSReductionQt5 ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
