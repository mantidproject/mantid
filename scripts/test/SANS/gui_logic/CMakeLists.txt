##
## Tests for SANS
##

set ( TEST_PY_FILES
  gui_state_director_test.py
  gui_common_test.py
  main_presenter_test.py
  property_manager_service_test.py
  run_tab_presenter_test.py
  state_gui_model_test.py
  settings_diagnostic_presenter_test.py
  table_model_test.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonSANS ${TEST_PY_FILES} )