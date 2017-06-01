##
## Tests for PythonAlgorithms
##

add_subdirectory( WorkflowAlgorithms )

set ( TEST_PY_FILES
  AbinsBasicTest.py
  AbinsAdvancedParametersTest.py
  AlignComponentsTest.py
  AngularAutoCorrelationsSingleAxisTest.py
  AngularAutoCorrelationsTwoAxesTest.py
  BinWidthAtXTest.py
  CalculateSampleTransmissionTest.py
  CheckForSampleLogsTest.py
  ConjoinSpectraTest.py
  CompareSampleLogsTest.py
  ComputeCalibrationCoefVanTest.py
  CorrectLogTimesTest.py
  CreateLeBailFitInputTest.py
  CorrectTOFTest.py
  CreateWorkspaceTest.py
  CylinderPaalmanPingsCorrection2Test.py
  DakotaChiSquaredTest.py
  DeltaPDF3DTest.py
  DNSComputeDetEffCorrCoefsTest.py
  DNSMergeRunsTest.py
  DNSFlippingRatioCorrTest.py
  DSFinterpTest.py
  EnggCalibrateFullTest.py
  EnggCalibrateTest.py
  EnggFitDIFCFromPeaksTest.py
  EnggFitPeaksTest.py
  EnggFocusTest.py
  EnggVanadiumCorrectionsTest.py
  ExportSpectraMaskTest.py   
  FilterLogByTimeTest.py
  FindEPPTest.py
  FindReflectometryLinesTest.py
  FitGaussianTest.py
  GetQsInQENSDataTest.py
  GSASIIRefineFitPeaksTest.py
  GetEiT0atSNSTest.py
  GetNegMuMuonicXRDTest.py
  IndirectTransmissionTest.py
  LoadDNSLegacyTest.py
  LoadEmptyVesuvioTest.py
  LoadFullprofFileTest.py
  LoadLiveDataTest.py
  LoadLogPropertyTableTest.py
  LoadMultipleGSSTest.py
  LoadNMoldyn3AsciiTest.py
  LoadNMoldyn4Ascii1DTest.py
  LoadNMoldyn4AsciiTest.py
  MaskAngleTest.py
  MaskBTPTest.py
  MaskWorkspaceToCalFileTest.py
  MatchPeaksTest.py
  MeanTest.py
  MedianBinWidthTest.py
  MergeCalFilesTest.py
  MuscatSofQWTest.py
  NMoldyn4InterpolationTest.py
  NormaliseSpectraTest.py
  RetrieveRunInfoTest.py
  SANSWideAngleCorrectionTest.py
  SaveNexusPDTest.py
  SavePlot1DAsJsonTest.py
  SaveVulcanGSSTest.py
  ExtractMonitorsTest.py
  SetDetScaleTest.py
  SortByQVectorsTest.py
  SortDetectorsTest.py
  SortXAxisTest.py
  StatisticsOfTableWorkspaceTest.py
  StringToPngTest.py
  SuggestTibCNCSTest.py
  SuggestTibHYSPECTest.py
  SymmetriseTest.py
  UpdatePeakParameterTableValueTest.py
  SANSSubtractTest.py
  TOFTOFCropWorkspaceTest.py
  TOFTOFMergeRunsTest.py
  ExportSampleLogsToCSVFileTest.py
  ExportExperimentLogTest.py
  PoldiMergeTest.py
  VelocityCrossCorrelationsTest.py
  VelocityAutoCorrelationsTest.py
  SelectNexusFilesByMetadataTest.py
  VesuvioPeakPredictionTest.py
  VesuvioPreFitTest.py
  VesuvioResolutionTest.py
  VesuvioThicknessTest.py
  VesuvioTOFFitTest.py
  PoldiCreatePeaksFromFileTest.py
  LoadCIFTest.py
  SaveYDATest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithms ${TEST_PY_FILES} )
