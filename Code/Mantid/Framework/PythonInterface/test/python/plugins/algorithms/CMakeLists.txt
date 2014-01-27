##
## Tests for PythonAlgorithms
##

set ( TEST_PY_FILES
  CheckForSampleLogsTest.py
  ConjoinSpectraTest.py
  CreateLeBailFitInputTest.py
  CreateWorkspaceTest.py
  DakotaChiSquaredTest.py
  FilterLogByTimeTest.py
  FindReflectometryLinesTest.py
  LoadFullprofFileTest.py
  LoadLiveDataTest.py
  LoadLogPropertyTableTest.py
  LoadMultipleGSSTest.py
  MaskWorkspaceToCalFileTest.py
  MeanTest.py
  MergeCalFilesTest.py
  RetrieveRunInfoTest.py
  SANSWideAngleCorrectionTest.py
  SortXAxisTest.py
  Stitch1DTest.py
  Stitch1DManyTest.py
  SuggestTibCNCSTest.py
  SuggestTibHYSPECTest.py
  UpdatePeakParameterTableValueTest.py
  SANSSubtractTest.py
  #FixGSASInstrumentFileTest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithms ${TEST_PY_FILES} )
