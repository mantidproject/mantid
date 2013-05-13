##
## This directory (and subdirectories) contains
## Python tests for each subpackage and the PythonAlgorithms
## 
if (PYUNITTEST_FOUND) 
  add_subdirectory( mantid )
  add_subdirectory( plugins )
endif ()
