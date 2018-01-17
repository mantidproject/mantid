##
## mantid.dataobjects tests
##
set ( TEST_PY_FILES
  functionsTest.py
  plots__init__Test.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test=PythonInterfacePlots
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} python.plots ${TEST_PY_FILES} )
