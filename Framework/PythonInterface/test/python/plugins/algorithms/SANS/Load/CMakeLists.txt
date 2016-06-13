##
## Tests for SANS
##

set ( TEST_PY_FILES
    SANSFileInformationTest.py
    SANSStatePrototypeTest.py
    SANSLoadTest.py
  )

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithmsSANS ${TEST_PY_FILES} )

