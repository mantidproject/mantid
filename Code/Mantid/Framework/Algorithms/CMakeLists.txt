set ( SRC_FILES
	src/AbsorptionCorrection.cpp
	src/AddSampleLog.cpp
	src/AlignDetectors.cpp
	src/AnyShapeAbsorption.cpp
	src/ApplyTransmissionCorrection.cpp
	src/BinaryOperation.cpp
	src/CalculateEfficiency.cpp
	src/CalculateTransmission.cpp
	src/CalculateTransmissionBeamSpreader.cpp
	src/CaltoDspacemap.cpp
	src/ChangeBinOffset.cpp
	src/ChangePulsetime.cpp
	src/CheckWorkspacesMatch.cpp
	src/ChopData.cpp
	src/CloneWorkspace.cpp
	src/CommutativeBinaryOperation.cpp
	src/ConjoinWorkspaces.cpp
	src/ConvertFromDistribution.cpp
	src/ConvertSpectrumAxis.cpp
	src/ConvertToDistribution.cpp
	src/ConvertToHistogram.cpp
	src/ConvertToMatrixWorkspace.cpp
	src/ConvertToPointData.cpp
	src/ConvertUnits.cpp
	src/CorrectKiKf.cpp
	src/CorrectToFile.cpp
	src/CreateCalFileByNames.cpp
	src/CreateDummyCalFile.cpp
	src/CreateGroupingWorkspace.cpp
	src/CreatePSDBleedMask.cpp
	src/CreatePeaksWorkspace.cpp
	src/CreateSingleValuedWorkspace.cpp
	src/CreateWorkspace.cpp
	src/CropWorkspace.cpp
	src/CrossCorrelate.cpp
	src/CuboidGaugeVolumeAbsorption.cpp
	src/CylinderAbsorption.cpp
	src/DampSq.cpp
	src/DeleteWorkspace.cpp
	src/DetectorDiagnostic.cpp
	src/DetectorEfficiencyCor.cpp
	src/DetectorEfficiencyVariation.cpp
	src/DiffractionEventCalibrateDetectors.cpp
	src/DiffractionEventReadDetCal.cpp
	src/DiffractionFocussing.cpp
	src/DiffractionFocussing2.cpp
	src/Divide.cpp
	src/DspacemaptoCal.cpp
	src/EQSANSTofStructure.cpp
	src/ElasticWindow.cpp
	src/Exponential.cpp
	src/ExponentialCorrection.cpp
	src/ExtractFFTSpectrum.cpp
	src/ExtractMasking.cpp
	src/ExtractSingleSpectrum.cpp
	src/FFT.cpp
	src/FFTDerivative.cpp
	src/FFTSmooth.cpp
	src/FFTSmooth2.cpp
	src/FilterBadPulses.cpp
	src/FilterByLogValue.cpp
	src/FilterByTime.cpp
	src/FindCenterOfMassPosition.cpp
	src/FindCenterOfMassPosition2.cpp
	src/FindDeadDetectors.cpp
	src/FindDetectorsOutsideLimits.cpp
	src/FindPeaks.cpp
	src/FlatBackground.cpp
	src/FlatPlateAbsorption.cpp
	src/GeneralisedSecondDifference.cpp
	src/GetDetectorOffsets.cpp
	src/GetEi.cpp
	src/GetEi2.cpp
	src/GhostCorrection.cpp
	src/GravitySANSHelper.cpp
	src/GroupWorkspaces.cpp
	src/HRPDSlabCanAbsorption.cpp
	src/He3TubeEfficiency.cpp
	src/IQTransform.cpp
	src/IdentifyNoisyDetectors.cpp
	src/InputWSDetectorInfo.cpp
	src/Integration.cpp
	src/InterpolatingRebin.cpp
	src/Logarithm.cpp
	src/MaskBins.cpp
	src/MaskDetectorsIf.cpp
	src/Max.cpp
	src/MedianDetectorTest.cpp
	src/MergeRuns.cpp
	src/Minus.cpp
	src/MonteCarloAbsorption.cpp
	src/MultipleScatteringAbsorption.cpp
	src/Multiply.cpp
	src/MultiplyRange.cpp
	src/MuonAlphaCalc.cpp
	src/MuonAsymmetryCalc.cpp
	src/MuonRemoveExpDecay.cpp
	src/NormaliseByCurrent.cpp
	src/NormaliseToMonitor.cpp
	src/OneMinusExponentialCor.cpp
	src/PDFFT.cpp
	src/PlotAsymmetryByLogValue.cpp
	src/Plus.cpp
	src/PointByPointVCorrection.cpp
	src/PoissonErrors.cpp
	src/PolynomialCorrection.cpp
	src/Power.cpp
	src/PowerLawCorrection.cpp
	src/Q1D.cpp
	src/Q1DWeighted.cpp
	src/Qxy.cpp
	src/ReadGroupsFromFile.cpp
	src/RealFFT.cpp
	src/Rebin.cpp
	src/RebinToWorkspace.cpp
	src/Rebunch.cpp
	src/Regroup.cpp
	src/RemoveBins.cpp
	src/RemoveLowResTOF.cpp
	src/RenameWorkspace.cpp
	src/ReplaceSpecialValues.cpp
	src/SANSDirectBeamScaling.cpp
	src/SANSSolidAngleCorrection.cpp
	src/Scale.cpp
	src/SetUncertaintiesToZero.cpp
	src/SmoothData.cpp
	src/SmoothNeighbours.cpp
	src/SofQW.cpp
	src/SolidAngle.cpp
	src/Sort.cpp
	src/SortEvents.cpp
	src/SpatialGrouping.cpp
	src/StripPeaks.cpp
	src/StripVanadiumPeaks.cpp
	src/SumNeighbours.cpp
	src/SumRowColumn.cpp
	src/SumSpectra.cpp
	src/Transpose.cpp
	src/UnGroupWorkspaces.cpp
	src/UnaryOperation.cpp
	src/Unwrap.cpp
	src/UnwrapMonitor.cpp
	src/UnwrapSNS.cpp
	src/WeightedMean.cpp
	src/XDataConverter.cpp
)

set ( INC_FILES
	inc/MantidAlgorithms/AbsorptionCorrection.h
	inc/MantidAlgorithms/AddSampleLog.h
	inc/MantidAlgorithms/AlignDetectors.h
	inc/MantidAlgorithms/AnyShapeAbsorption.h
	inc/MantidAlgorithms/ApplyTransmissionCorrection.h
	inc/MantidAlgorithms/BinaryOperation.h
	inc/MantidAlgorithms/CalculateEfficiency.h
	inc/MantidAlgorithms/CalculateTransmission.h
	inc/MantidAlgorithms/CalculateTransmissionBeamSpreader.h
	inc/MantidAlgorithms/CaltoDspacemap.h
	inc/MantidAlgorithms/ChangeBinOffset.h
	inc/MantidAlgorithms/ChangePulsetime.h
	inc/MantidAlgorithms/CheckWorkspacesMatch.h
	inc/MantidAlgorithms/ChopData.h
	inc/MantidAlgorithms/CloneWorkspace.h
	inc/MantidAlgorithms/CommutativeBinaryOperation.h
	inc/MantidAlgorithms/ConjoinWorkspaces.h
	inc/MantidAlgorithms/ConvertFromDistribution.h
	inc/MantidAlgorithms/ConvertSpectrumAxis.h
	inc/MantidAlgorithms/ConvertToDistribution.h
	inc/MantidAlgorithms/ConvertToHistogram.h
	inc/MantidAlgorithms/ConvertToMatrixWorkspace.h
	inc/MantidAlgorithms/ConvertToPointData.h
	inc/MantidAlgorithms/ConvertUnits.h
	inc/MantidAlgorithms/CorrectKiKf.h
	inc/MantidAlgorithms/CorrectToFile.h
	inc/MantidAlgorithms/CreateCalFileByNames.h
	inc/MantidAlgorithms/CreateDummyCalFile.h
	inc/MantidAlgorithms/CreateGroupingWorkspace.h
	inc/MantidAlgorithms/CreatePSDBleedMask.h
	inc/MantidAlgorithms/CreatePeaksWorkspace.h
	inc/MantidAlgorithms/CreateSingleValuedWorkspace.h
	inc/MantidAlgorithms/CreateWorkspace.h
	inc/MantidAlgorithms/CropWorkspace.h
	inc/MantidAlgorithms/CrossCorrelate.h
	inc/MantidAlgorithms/CuboidGaugeVolumeAbsorption.h
	inc/MantidAlgorithms/CylinderAbsorption.h
	inc/MantidAlgorithms/DampSq.h
	inc/MantidAlgorithms/DeleteWorkspace.h
	inc/MantidAlgorithms/DetectorDiagnostic.h
	inc/MantidAlgorithms/DetectorEfficiencyCor.h
	inc/MantidAlgorithms/DetectorEfficiencyVariation.h
	inc/MantidAlgorithms/DiffractionEventCalibrateDetectors.h
	inc/MantidAlgorithms/DiffractionEventReadDetCal.h
	inc/MantidAlgorithms/DiffractionFocussing.h
	inc/MantidAlgorithms/DiffractionFocussing2.h
	inc/MantidAlgorithms/Divide.h
	inc/MantidAlgorithms/DspacemaptoCal.h
	inc/MantidAlgorithms/EQSANSTofStructure.h
	inc/MantidAlgorithms/ElasticWindow.h
	inc/MantidAlgorithms/Exponential.h
	inc/MantidAlgorithms/ExponentialCorrection.h
	inc/MantidAlgorithms/ExtractFFTSpectrum.h
	inc/MantidAlgorithms/ExtractMasking.h
	inc/MantidAlgorithms/ExtractSingleSpectrum.h
	inc/MantidAlgorithms/FFT.h
	inc/MantidAlgorithms/FFTDerivative.h
	inc/MantidAlgorithms/FFTSmooth.h
	inc/MantidAlgorithms/FFTSmooth2.h
	inc/MantidAlgorithms/FilterBadPulses.h
	inc/MantidAlgorithms/FilterByLogValue.h
	inc/MantidAlgorithms/FilterByTime.h
	inc/MantidAlgorithms/FindCenterOfMassPosition.h
	inc/MantidAlgorithms/FindCenterOfMassPosition2.h
	inc/MantidAlgorithms/FindDeadDetectors.h
	inc/MantidAlgorithms/FindDetectorsOutsideLimits.h
	inc/MantidAlgorithms/FindPeaks.h
	inc/MantidAlgorithms/FlatBackground.h
	inc/MantidAlgorithms/FlatPlateAbsorption.h
	inc/MantidAlgorithms/GSLFunctions.h
	inc/MantidAlgorithms/GeneralisedSecondDifference.h
	inc/MantidAlgorithms/GetDetectorOffsets.h
	inc/MantidAlgorithms/GetEi.h
	inc/MantidAlgorithms/GetEi2.h
	inc/MantidAlgorithms/GhostCorrection.h
	inc/MantidAlgorithms/GravitySANSHelper.h
	inc/MantidAlgorithms/GroupWorkspaces.h
	inc/MantidAlgorithms/HRPDSlabCanAbsorption.h
	inc/MantidAlgorithms/He3TubeEfficiency.h
	inc/MantidAlgorithms/IQTransform.h
	inc/MantidAlgorithms/IdentifyNoisyDetectors.h
	inc/MantidAlgorithms/InputWSDetectorInfo.h
	inc/MantidAlgorithms/Integration.h
	inc/MantidAlgorithms/InterpolatingRebin.h
	inc/MantidAlgorithms/Logarithm.h
	inc/MantidAlgorithms/MaskBins.h
	inc/MantidAlgorithms/MaskDetectorsIf.h
	inc/MantidAlgorithms/Max.h
	inc/MantidAlgorithms/MedianDetectorTest.h
	inc/MantidAlgorithms/MergeRuns.h
	inc/MantidAlgorithms/Minus.h
	inc/MantidAlgorithms/MonteCarloAbsorption.h
	inc/MantidAlgorithms/MultipleScatteringAbsorption.h
	inc/MantidAlgorithms/Multiply.h
	inc/MantidAlgorithms/MultiplyRange.h
	inc/MantidAlgorithms/MuonAlphaCalc.h
	inc/MantidAlgorithms/MuonAsymmetryCalc.h
	inc/MantidAlgorithms/MuonRemoveExpDecay.h
	inc/MantidAlgorithms/NormaliseByCurrent.h
	inc/MantidAlgorithms/NormaliseToMonitor.h
	inc/MantidAlgorithms/OneMinusExponentialCor.h
	inc/MantidAlgorithms/PDFFT.h
	inc/MantidAlgorithms/PlotAsymmetryByLogValue.h
	inc/MantidAlgorithms/Plus.h
	inc/MantidAlgorithms/PointByPointVCorrection.h
	inc/MantidAlgorithms/PoissonErrors.h
	inc/MantidAlgorithms/PolynomialCorrection.h
	inc/MantidAlgorithms/Power.h
	inc/MantidAlgorithms/PowerLawCorrection.h
	inc/MantidAlgorithms/Q1D.h
	inc/MantidAlgorithms/Q1DWeighted.h
	inc/MantidAlgorithms/Qxy.h
	inc/MantidAlgorithms/ReadGroupsFromFile.h
	inc/MantidAlgorithms/RealFFT.h
	inc/MantidAlgorithms/Rebin.h
	inc/MantidAlgorithms/RebinToWorkspace.h
	inc/MantidAlgorithms/Rebunch.h
	inc/MantidAlgorithms/Regroup.h
	inc/MantidAlgorithms/RemoveBins.h
	inc/MantidAlgorithms/RemoveLowResTOF.h
	inc/MantidAlgorithms/RenameWorkspace.h
	inc/MantidAlgorithms/ReplaceSpecialValues.h
	inc/MantidAlgorithms/SANSDirectBeamScaling.h
	inc/MantidAlgorithms/SANSSolidAngleCorrection.h
	inc/MantidAlgorithms/Scale.h
	inc/MantidAlgorithms/SetUncertaintiesToZero.h
	inc/MantidAlgorithms/SmoothData.h
	inc/MantidAlgorithms/SmoothNeighbours.h
	inc/MantidAlgorithms/SofQW.h
	inc/MantidAlgorithms/SolidAngle.h
	inc/MantidAlgorithms/Sort.h
	inc/MantidAlgorithms/SortEvents.h
	inc/MantidAlgorithms/SpatialGrouping.h
	inc/MantidAlgorithms/StripPeaks.h
	inc/MantidAlgorithms/StripVanadiumPeaks.h
	inc/MantidAlgorithms/SumNeighbours.h
	inc/MantidAlgorithms/SumRowColumn.h
	inc/MantidAlgorithms/SumSpectra.h
	inc/MantidAlgorithms/Transpose.h
	inc/MantidAlgorithms/UnGroupWorkspaces.h
	inc/MantidAlgorithms/UnaryOperation.h
	inc/MantidAlgorithms/Unwrap.h
	inc/MantidAlgorithms/UnwrapMonitor.h
	inc/MantidAlgorithms/UnwrapSNS.h
	inc/MantidAlgorithms/WeightedMean.h
	inc/MantidAlgorithms/XDataConverter.h
)

set ( TEST_FILES
	test/AddSampleLogTest.h
	test/AlignDetectorsTest.h
	test/AnyShapeAbsorptionTest.h
	test/ApplyTransmissionCorrectionTest.h
	test/BinaryOperationTest.h
	test/CalculateEfficiencyTest.h
	test/CalculateTransmissionBeamSpreaderTest.h
	test/CalculateTransmissionTest.h
	test/ChainedOperatorTest.h
	test/ChangeBinOffsetTest.h
	test/ChangePulsetimeTest.h
	test/CheckWorkspacesMatchTest.h
	test/ChopDataTest.h
	test/CloneWorkspaceTest.h
	test/CommutativeBinaryOperationTest.h
	test/CompressedRebinTest.h
	test/ConjoinWorkspacesTest.h
	test/ConvertFromDistributionTest.h
	test/ConvertSpectrumAxisTest.h
	test/ConvertToDistributionTest.h
	test/ConvertToHistogramTest.h
	test/ConvertToMatrixWorkspaceTest.h
	test/ConvertToPointDataTest.h
	test/ConvertUnitsTest.h
	test/CorrectKiKfTest.h
	test/CorrectToFileTest.h
	test/CreateCalFileByNamesTest.h
	test/CreateDummyCalFileTest.h
	test/CreateGroupingWorkspaceTest.h
	test/CreatePSDBleedMaskTest.h
	test/CreatePeaksWorkspaceTest.h
	test/CreateSingleValuedWorkspaceTest.h
	test/CreateWorkspaceTest.h
	test/CropWorkspaceTest.h
	test/CuboidGaugeVolumeAbsorptionTest.h
	test/CylinderAbsorptionTest.h
	test/DampSqTest.h
	test/DeleteWorkspaceTest.h
	test/DetectorEfficiencyCorTest.h
	test/DetectorEfficiencyVariationTest.h
	test/DiffractionEventReadDetCalTest.h
	test/DiffractionFocussing2Test.h
	test/DiffractionFocussingTest.h
	test/DivideTest.h
	test/DspacemaptoCalTest.h
	test/ExponentialCorrectionTest.h
	test/ExponentialTest.h
	test/ExtractFFTSpectrumTest.h
	test/ExtractMaskingTest.h
	test/ExtractSingleSpectrumTest.h
	test/FFTDerivativeTest.h
	test/FFTSmooth2Test.h
	test/FFTTest.h
  test/FilterBadPulsesTest.h
	test/FilterByLogValueTest.h
	test/FilterByTimeTest.h
	test/FindCenterOfMassPosition2Test.h
	test/FindCenterOfMassPositionTest.h
	test/FindDeadDetectorsTest.h
	test/FindDetectorsOutsideLimitsTest.h
	test/FindPeaksTest.h
	test/FlatBackgroundTest.h
	test/FlatPlateAbsorptionTest.h
	test/GetDetectorOffsetsTest.h
	test/GetEiTest.h
	test/GhostCorrectionTest.h
	test/GroupWorkspacesTest.h
	test/HRPDSlabCanAbsorptionTest.h
	test/He3TubeEfficiencyTest.h
	test/IQTransformTest.h
	test/IdentifyNoisyDetectorsTest.h
	test/IntegrationTest.h
	test/InterpolatingRebinTest.h
	test/LogarithmTest.h
	test/MaskBinsTest.h
	test/MedianDetectorTestTest.h
	test/MemoryLeakTest.h
	test/MergeRunsTest.h
	test/MinusTest.h
	test/MonteCarloAbsorptionTest.h
	test/MultipleScatteringAbsorptionTest.h
	test/MultiplyRangeTest.h
	test/MultiplyTest.h
	test/MuonAlphaCalcTest.h
	test/MuonAsymmetryCalcTest.h
	test/MuonRemoveExpDecayTest.h
	test/NormaliseByCurrentTest.h
	test/NormaliseToMonitorTest.h
	test/OneMinusExponentialCorTest.h
	test/PDFFTTest.h
	test/PlotAsymmetryByLogValueTest.h
	test/PlusTest.h
	test/PointByPointVCorrectionTest.h
	test/PoissonErrorsTest.h
	test/PolynomialCorrectionTest.h
	test/PowerLawCorrectionTest.h
	test/PowerTest.h
	test/Q1DTest.h
	test/Q1DWeightedTest.h
	test/QxyTest.h
	test/ReadGroupsFromFileTest.h
	test/RealFFTTest.h
	test/RebinTest.h
	test/RebinToWorkspaceTest.h
	test/RebunchTest.h
	test/RegroupTest.h
	test/RemoveBinsTest.h
	test/RemoveLowResTOFTest.h
	test/RenameWorkspaceTest.h
	test/ReplaceSpecialValuesTest.h
	test/SANSSolidAngleCorrectionTest.h
	test/SaveToSNSHistogramNexusTest.h
	test/ScaleTest.h
	test/SmoothDataTest.h
	test/SmoothNeighboursTest.h
	test/SofQWTest.h
	test/SolidAngleTest.h
	test/SortEventsTest.h
	test/SpatialGroupingTest.h
	test/StripPeaksTest.h
	test/StripVanadiumPeaksTest.h
	test/SumNeighboursTest.h
	test/SumRowColumnTest.h
	test/SumSpectraTest.h
	test/TransposeTest.h
	test/UnGroupWorkspacesTest.h
	test/UnaryOperationTest.h
	test/UnwrapSNSTest.h
	test/UnwrapTest.h
	test/WeightedMeanTest.h
	test/WorkspaceGroupTest.h
)

set(SRC_UNITY_IGNORE_FILES src/AlignDetectors.cpp
    src/FFTSmooth.cpp 
    src/FFTSmooth2.cpp
    src/FilterBadPulses.cpp
    src/SetUncertaintiesToZero.cpp
    )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Algorithms SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( Algorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Algorithms PROPERTIES OUTPUT_NAME MantidAlgorithms )
# Add to the 'Framework' group in VS
set_property ( TARGET Algorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( Algorithms ${MANTIDLIBS} ${GSL_LIBRARIES} )

if ( CXXTEST_FOUND )
  # Generate the MultiplyTest.h and DivideTest.h
  set ( MULTIPLYDIVIDETEST_CLASS MultiplyTest )
  set ( MULTIPLYDIVIDETEST_DO_DIVIDE false )
  configure_file ( test/MultiplyDivideTest.h.in      test/MultiplyTest.h)
  set ( MULTIPLYDIVIDETEST_CLASS DivideTest )
  set ( MULTIPLYDIVIDETEST_DO_DIVIDE true )
  configure_file ( test/MultiplyDivideTest.h.in      test/DivideTest.h)

  # Generate the PlusTest.h and MinusTest.h
  set ( PLUSMINUSTEST_CLASS PlusTest )
  set ( PLUSMINUSTEST_DO_PLUS true )
  configure_file ( test/PlusMinusTest.h.in      test/PlusTest.h)
  set ( PLUSMINUSTEST_CLASS MinusTest )
  set ( PLUSMINUSTEST_DO_PLUS false )
  configure_file ( test/PlusMinusTest.h.in      test/MinusTest.h)
  
  # Make the rest of the tests
  include_directories ( ../CurveFitting/inc ../DataHandling/inc ../Nexus/inc ../TestHelpers/inc )
  cxxtest_add_test ( AlgorithmsTest ${TEST_FILES} )
  target_link_libraries ( AlgorithmsTest Algorithms CurveFitting DataHandling Nexus TestHelpers )
  add_dependencies ( FrameworkTests AlgorithmsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET AlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Algorithms DESTINATION ${PLUGINS_DIR} )
