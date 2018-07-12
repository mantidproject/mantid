##
## Tests for PythonWorkflowAlgorithms
##

set ( TEST_PY_FILES
  AddSampleLogMultipleTest.py
  ApplyPaalmanPingsCorrectionTest.py
  BayesQuasiTest.py
  BayesStretchTest.py
  CalculateMonteCarloAbsorptionTest.py
  DetectorFloodWeightingTest.py
  DirectILLApplySelfShieldingTest.py
  DirectILLCollectDataTest.py
  DirectILLDiagnosticsTest.py
  DirectILLIntegrateVanadiumTest.py
  DirectILLReductionTest.py
  DirectILLSelfShieldingTest.py
  EnergyWindowScanTest.py
  FlatPlatePaalmanPingsCorrectionTest.py
  IndirectAnnulusAbsorptionTest.py
  IndirectAnnulusAbsorption2Test.py
  IndirectCalibrationTest.py
  IndirectCylinderAbsorptionTest.py
  IndirectCylinderAbsorption2Test.py
  IndirectDiffScanTest.py
  IndirectFlatPlateAbsorptionTest.py
  IndirectFlatPlateAbsorption2Test.py
  IndirectILLEnergyTransferTest.py
  IndirectILLReductionFWSTest.py
  IndirectILLReductionQENSTest.py
  IndirectResolutionTest.py
  IndirectSampleChangerTest.py
  IndirectTransmissionMonitorTest.py
  IqtFitMultipleTest.py
  IqtFitSequentialTest.py
  ISISIndirectDiffractionReductionTest.py
  ISISIndirectEnergyTransferTest.py
  LoadWANDTest.py
  MDNormSCDPreprocessIncoherentTest.py
  MolDynTest.py
  MSDFitTest.py
  OSIRISDiffractionReductionTest.py
  PowderDiffILLReductionTest.py
  ReflectometryILLConvertToQTest.py
  ReflectometryILLPolarizationCorTest.py
  ReflectometryILLPreprocessTest.py
  ReflectometryILLSumForegroundTest.py
  ResNorm2Test.py
  SANSDarkRunBackgroundCorrectionTest.py
  SANSFitShiftScaleTest.py
  SANSMaskTest.py
  SANSStitchTest.py
  SavePlot1DTest.py
  SaveVulcanGSSTest.py
  SimpleShapeMonteCarloAbsorptionTest.py
  SimulatedDensityOfStatesTest.py
  SofQWMomentsTest.py
  SofQWMomentsScanTest.py
  SwapWidthsTest.py
  TimeSliceTest.py
  TOSCABankCorrectionTest.py
  TransformToIqtTest.py
  VesuvioDiffractionReductionTest.py
  WANDPowderReductionTest.py
)
check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonWorkflowAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} python.WorkflowAlgorithms ${TEST_PY_FILES} )
add_subdirectory( sans )
