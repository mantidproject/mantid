##
## Tests for PythonWorkflowAlgorithms
##

set ( TEST_PY_FILES
  AddSampleLogMultipleTest.py
  ApplyPaalmanPingsCorrectionTest.py
  BayesQuasiTest.py
  BayesStretchTest.py
  DetectorFloodWeightingTest.py
  EnergyWindowScanTest.py
  FlatPlatePaalmanPingsCorrectionTest.py
  ILLIN16BCalibrationTest.py
  IndirectAnnulusAbsorptionTest.py
  IndirectAnnulusAbsorption2Test.py
  IndirectCalibrationTest.py
  IndirectCylinderAbsorptionTest.py
  IndirectCylinderAbsorption2Test.py
  IndirectDiffScanTest.py
  IndirectFlatPlateAbsorptionTest.py
  IndirectFlatPlateAbsorption2Test.py
  IndirectILLEnergyTransferTest.py
  IndirectILLReductionTest.py
  IndirectILLReductionFWSTest.py
  IndirectILLReductionQENSTest.py
  IndirectResolutionTest.py
  IndirectSampleChangerTest.py
  IndirectTransmissionMonitorTest.py
  IqtFitMultipleTest.py
  IqtFitSequentialTest.py
  ISISIndirectDiffractionReductionTest.py
  ISISIndirectEnergyTransferTest.py
  MolDynTest.py
  MSDFitTest.py
  OSIRISDiffractionReductionTest.py
  ResNorm2Test.py
  SANSDarkRunBackgroundCorrectionTest.py
  SANSFitShiftScaleTest.py
  SANSStitchTest.py
  SavePlot1DTest.py
  SimulatedDensityOfStatesTest.py
  SofQWMomentsTest.py
  SofQWMomentsScanTest.py
  SwapWidthsTest.py
  TimeSliceTest.py
  TOSCABankCorrectionTest.py
  TransformToIqtTest.py
  VesuvioDiffractionReductionTest.py
)
check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonWorkflowAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonWorkflowAlgorithms ${TEST_PY_FILES} )
add_subdirectory( sans )
