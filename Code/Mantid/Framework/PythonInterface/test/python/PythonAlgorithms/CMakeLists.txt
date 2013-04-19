##
## Tests for PythonAlgorithms
##

set ( TEST_PY_FILES
  CheckForSampleLogsTest.py
  SuggestTibCNCSTest.py
  SuggestTibHYSPECTest.py
)

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithms ${TEST_PY_FILES} )
