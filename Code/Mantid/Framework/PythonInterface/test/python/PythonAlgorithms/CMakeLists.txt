##
## Tests for PythonAlgorithms
##

file(GLOB TEST_PY_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}  "*.py" )

if ( TEST_PY_FILES )
  # Prefix for test name=PythonAlgorithms
  pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithms ${TEST_PY_FILES} )
endif()
