#
## Tests for Muon GUIs
##

set ( TEST_PY_FILES
   FFTPresenter_test.py
   FFTModel_test.py
   MaxEntPresenter_test.py
   MaxEntModel_test.py
   transformWidget_test.py
   PeriodicTablePresenter_test.py
   PeriodicTableModel_test.py
   LoadWidgetPresenter_test.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} python.Muon ${TEST_PY_FILES} )
