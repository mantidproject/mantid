##
## Tests for SANS
##

set ( TEST_PY_FILES
  calculate_transmission_helper_test.py
  q_resolution_calculator_test.py
  scale_helper_test.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonSANS
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonSANS ${TEST_PY_FILES} )
