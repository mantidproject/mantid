##
## Tests for PythonAlgorithms
##

set ( TEST_PY_FILES
  CheckForSampleLogsTest.py
  ConjoinSpectraTest.py
  CreateLeBailFitInputTest.py
  CreateWorkspaceTest.py
  DakotaChiSquaredTest.py
  FindReflectometryLinesTest.py
  LoadFullprofFileTest.py
  LoadLiveDataTest.py
  LoadLogPropertyTableTest.py
  LoadMultipleGSSTest.py
  MaskWorkspaceToCalFileTest.py
  MergeCalFilesTest.py
  RetrieveRunInfoTest.py
  RunPythonScriptTest.py
  SANSWideAngleCorrectionTest.py
  Stitch1DTest.py
  SuggestTibCNCSTest.py
  SuggestTibHYSPECTest.py
  UpdatePeakParameterTableValueTest.py
  SANSSubtractTest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithms ${TEST_PY_FILES} )
