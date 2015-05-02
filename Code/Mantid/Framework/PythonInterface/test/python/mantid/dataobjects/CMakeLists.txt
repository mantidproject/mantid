##
## mantid.dataobjects tests
##
set ( TEST_PY_FILES
  Workspace2DTest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test=PythonInterfaceDataObjects
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonInterfaceDataObjects ${TEST_PY_FILES} )
