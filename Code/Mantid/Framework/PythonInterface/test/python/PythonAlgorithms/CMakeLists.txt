##
## Tests for PythonAlgorithms
##

set ( TEST_PY_FILES
  CreateWorkspaceTest.py
  FindReflectometryLinesTest.py
  MaskWorkspaceToCalFileTest.py
  RetrieveRunInfoTest.py
  Stitch1DTest.py
)

# Prefix for test name=PythonAlgorithms
pyunittest_add_test_two ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithms ${TEST_PY_FILES} )
