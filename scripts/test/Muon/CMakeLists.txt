#
## Tests for Muon GUIs
##

set ( TEST_PY_FILES
   AxisChangerPresenter_test.py
   AxisChangerView_test.py
   FFTModel_test.py
   FFTPresenter_test.py
   LoadWidgetModel_test.py
   LoadWidgetPresenter_test.py
   LoadWidgetView_test.py
   MaxEntModel_test.py
   MaxEntPresenter_test.py
   PeriodicTableModel_test.py
   PeriodicTablePresenter_test.py
   PlottingPresenter_test.py
   PlottingUtils_test.py
   PlottingView_test.py
   transformWidget_test.py
   utilities/thread_model_test.py
   utilities/load_utils_test.py
   utilities/muon_workspace_wrapper_test.py
   utilities/muon_workspace_wrapper_directory_test.py
   utilities/muon_load_data_test.py
   utilities/muon_file_utils_test.py
   utilities/run_string_utils_operator_test.py
   utilities/run_string_utils_conversion_test.py
   subplotObject_test.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
set ( PYUNITTEST_QT_API pyqt ) # force to use qt4
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} python.MuonQt4 ${TEST_PY_FILES} )
unset ( PYUNITTEST_QT_API )
