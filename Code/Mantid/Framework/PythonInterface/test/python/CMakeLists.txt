##
## This directory (and subdirectories) contains
## Python tests for each subpackage and the PythonAlgorithms
## 
if (PYUNITTEST_FOUND) 
  # pyunittest_add_test understands this macro and adds it to the PYTHONPATH for each test
  set ( TESTHELPER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

  add_subdirectory( mantid )
  add_subdirectory( plugins )
endif ()
