##
## Tests for PythonAlgorithms
##

set ( TEST_PY_FILES
  CheckForSampleLogsTest.py
  ConjoinSpectraTest.py
  CorrectLogTimesTest.py
  CreateLeBailFitInputTest.py
  CreateCalibrationWorkspaceTest.py
  CreateWorkspaceTest.py
  DakotaChiSquaredTest.py
  DensityOfStatesTest.py
  DSFinterpTest.py
  FilterLogByTimeTest.py
  FindReflectometryLinesTest.py
  GetEiT0atSNSTest.py
  IndirectILLReductionTest.py
  IndirectTransmissionTest.py
  IndirectTransmissionMonitorTest.py
  ISISIndirectEnergyTransferTest.py
  LoadFullprofFileTest.py
  LoadLiveDataTest.py
  LoadLogPropertyTableTest.py
  LoadMultipleGSSTest.py
  MaskAngleTest.py
  MaskBTPTest.py
  MaskWorkspaceToCalFileTest.py
  MeanTest.py
  MergeCalFilesTest.py
  MolDynTest.py
  PDDetermineCharacterizationsTest.py
  RetrieveRunInfoTest.py
  SANSWideAngleCorrectionTest.py
  SavePlot1DTest.py
  SaveVulcanGSSTest.py
  SortByQVectorsTest.py
  SofQWMomentsTest.py
  SortDetectorsTest.py
  SortXAxisTest.py
  Stitch1DManyTest.py
  SuggestTibCNCSTest.py
  SuggestTibHYSPECTest.py
  SymmetriseTest.py
  UpdatePeakParameterTableValueTest.py
  SANSSubtractTest.py
  TimeSliceTest.py
  ExportSampleLogsToCSVFileTest.py
  ExportExperimentLogTest.py
  PoldiMergeTest.py
  VesuvioResolutionTest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithms ${TEST_PY_FILES} )
