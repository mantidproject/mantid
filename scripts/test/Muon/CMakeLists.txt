#
## Tests for Muon GUIs
##

set ( TEST_PY_FILES
   FFTPresenter_test.py
   FFTModel_test.py
   loadfile_model_test.py
   loadfile_presenter_test.py
   loadfile_view_test.py
   MaxEntPresenter_test.py
   MaxEntModel_test.py
   muon_file_utils_test.py
   muon_load_data_test.py
   run_string_utils_test.py
   transformWidget_test.py
   PeriodicTablePresenter_test.py
   PeriodicTableModel_test.py
   LoadWidgetPresenter_test.py
   LoadWidgetModel_test.py
   LoadWidgetView_test.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} python.Muon ${TEST_PY_FILES} )
