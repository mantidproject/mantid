set ( SRC_FILES src/AbsorptionCorrection.cpp
                src/AddSampleLog.cpp
		src/AlignDetectors.cpp
		src/AnyShapeAbsorption.cpp
		src/ApplyTransmissionCorrection.cpp
		src/BinaryOperation.cpp
		src/CalculateEfficiency.cpp
		src/CalculateTransmissionBeamSpreader.cpp
		src/CalculateTransmission.cpp
		src/ChangeBinOffset.cpp
		src/CheckWorkspacesMatch.cpp
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
		src/CreateSingleValuedWorkspace.cpp
		src/CreateWorkspace.cpp
		src/CropWorkspace.cpp
		src/CrossCorrelate.cpp
		src/CuboidGaugeVolumeAbsorption.cpp
		src/CylinderAbsorption.cpp
		src/DetectorDiagnostic.cpp
		src/DetectorEfficiencyCor.cpp
		src/DetectorEfficiencyVariation.cpp
		src/DiffractionEventCalibrateDetectors.cpp
		src/DiffractionEventReadDetCal.cpp
		src/DiffractionFocussing2.cpp
		src/DiffractionFocussing.cpp
		src/Divide.cpp
		src/DspacemaptoCal.cpp
		src/ElasticWindow.cpp
		src/EQSANSTofStructure.cpp
		src/ExponentialCorrection.cpp
		src/Exponential.cpp
		src/ExtractFFTSpectrum.cpp
		src/ExtractMasking.cpp
		src/ExtractSingleSpectrum.cpp
		src/FFT.cpp
		src/FFTSmooth2.cpp
		src/FFTSmooth.cpp
		src/FilterBadPulses.cpp
		src/FilterByLogValue.cpp
		src/FilterByTime.cpp
		src/FindCenterOfMassPosition.cpp
		src/FindDeadDetectors.cpp
		src/FindDetectorsOutsideLimits.cpp
		src/FindPeaks.cpp
		src/FlatBackground.cpp
		src/FlatPlateAbsorption.cpp
		src/GeneralisedSecondDifference.cpp
		src/GetDetectorOffsets.cpp
		src/GetEi2.cpp
		src/GetEi.cpp
		src/GhostCorrection.cpp
		src/GravitySANSHelper.cpp
		src/GroupWorkspaces.cpp
		src/He3TubeEfficiency.cpp
		src/HRPDSlabCanAbsorption.cpp
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
		src/RenameWorkspace.cpp
		src/ReplaceSpecialValues.cpp
		src/Scale.cpp
		src/SetUncertaintiesToZero.cpp
		src/SmoothData.cpp
		src/SofQW.cpp
		src/SolidAngleCorrection.cpp
		src/SolidAngle.cpp
		src/Sort.cpp
		src/SpatialGrouping.cpp
		src/StripPeaks.cpp
		src/StripVanadiumPeaks.cpp
		src/SumNeighbours.cpp
		src/SumRowColumn.cpp
		src/SumSpectra.cpp
		src/TofCorrection.cpp
		src/Transpose.cpp
		src/UnaryOperation.cpp
		src/UnGroupWorkspaces.cpp
		src/Unwrap.cpp
		src/WeightedMean.cpp
		src/XDataConverter.cpp )

set ( INC_FILES inc/MantidAlgorithms/AbsorptionCorrection.h
                inc/MantidAlgorithms/AddSampleLog.h
		inc/MantidAlgorithms/AlignDetectors.h
		inc/MantidAlgorithms/AnyShapeAbsorption.h
		inc/MantidAlgorithms/ApplyTransmissionCorrection.h
		inc/MantidAlgorithms/BinaryOperation.h
		inc/MantidAlgorithms/CalculateEfficiency.h
		inc/MantidAlgorithms/CalculateTransmissionBeamSpreader.h
		inc/MantidAlgorithms/CalculateTransmission.h
		inc/MantidAlgorithms/ChangeBinOffset.h
		inc/MantidAlgorithms/CheckWorkspacesMatch.h
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
		inc/MantidAlgorithms/CreateSingleValuedWorkspace.h
		inc/MantidAlgorithms/CreateWorkspace.h
		inc/MantidAlgorithms/CropWorkspace.h
		inc/MantidAlgorithms/CrossCorrelate.h
		inc/MantidAlgorithms/CuboidGaugeVolumeAbsorption.h
		inc/MantidAlgorithms/CylinderAbsorption.h
		inc/MantidAlgorithms/DetectorDiagnostic.h
		inc/MantidAlgorithms/DetectorEfficiencyCor.h
		inc/MantidAlgorithms/DetectorEfficiencyVariation.h
		inc/MantidAlgorithms/DiffractionEventCalibrateDetectors.h
		inc/MantidAlgorithms/DiffractionEventReadDetCal.h
		inc/MantidAlgorithms/DiffractionFocussing2.h
		inc/MantidAlgorithms/DiffractionFocussing.h
		inc/MantidAlgorithms/Divide.h
		inc/MantidAlgorithms/DspacemaptoCal.h
		inc/MantidAlgorithms/ElasticWindow.h
		inc/MantidAlgorithms/EQSANSTofStructure.h
		inc/MantidAlgorithms/ExponentialCorrection.h
		inc/MantidAlgorithms/Exponential.h
		inc/MantidAlgorithms/ExtractFFTSpectrum.h
		inc/MantidAlgorithms/ExtractMasking.h
		inc/MantidAlgorithms/ExtractSingleSpectrum.h
		inc/MantidAlgorithms/FFT.h
		inc/MantidAlgorithms/FFTSmooth2.h
		inc/MantidAlgorithms/FFTSmooth.h
		inc/MantidAlgorithms/FilterBadPulses.h
		inc/MantidAlgorithms/FilterByLogValue.h
		inc/MantidAlgorithms/FilterByTime.h
		inc/MantidAlgorithms/FindCenterOfMassPosition.h
		inc/MantidAlgorithms/FindDeadDetectors.h
		inc/MantidAlgorithms/FindDetectorsOutsideLimits.h
		inc/MantidAlgorithms/FindPeaks.h
		inc/MantidAlgorithms/FlatBackground.h
		inc/MantidAlgorithms/FlatPlateAbsorption.h
		inc/MantidAlgorithms/GeneralisedSecondDifference.h
		inc/MantidAlgorithms/GetDetectorOffsets.h
		inc/MantidAlgorithms/GetEi2.h
		inc/MantidAlgorithms/GetEi.h
		inc/MantidAlgorithms/GhostCorrection.h
		inc/MantidAlgorithms/GravitySANSHelper.h
		inc/MantidAlgorithms/GroupWorkspaces.h
		inc/MantidAlgorithms/GSLFunctions.h
		inc/MantidAlgorithms/He3TubeEfficiency.h
		inc/MantidAlgorithms/HRPDSlabCanAbsorption.h
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
		inc/MantidAlgorithms/RenameWorkspace.h
		inc/MantidAlgorithms/ReplaceSpecialValues.h
		inc/MantidAlgorithms/Scale.h
		inc/MantidAlgorithms/SetUncertaintiesToZero.h
		inc/MantidAlgorithms/SmoothData.h
		inc/MantidAlgorithms/SofQW.h
		inc/MantidAlgorithms/SolidAngleCorrection.h
		inc/MantidAlgorithms/SolidAngle.h
		inc/MantidAlgorithms/Sort.h
		inc/MantidAlgorithms/SpatialGrouping.h
		inc/MantidAlgorithms/StripPeaks.h
		inc/MantidAlgorithms/StripVanadiumPeaks.h
		inc/MantidAlgorithms/SumNeighbours.h
		inc/MantidAlgorithms/SumRowColumn.h
		inc/MantidAlgorithms/SumSpectra.h
		inc/MantidAlgorithms/TofCorrection.h )

set ( TEST_FILES test/AddSampleLogTest.h
                 test/AlignDetectorsTest.h
		 #test/AnyShapeAbsorptionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/ApplyTransmissionCorrectionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/BinaryOperationTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/CalculateEfficiencyTest.h # TODO uses ComponentCreationHelper.hh via SANSInstrumentCreationHelper.h
		 #test/CalculateTransmissionBeamSpreaderTest.h # TODO uses ComponentCreationHelper.hh via SANSInstrumentCreationHelper.h
		 #test/CalculateTransmissionTest.h # TODO uses ComponentCreationHelper.hh via SANSInstrumentCreationHelper.h
		 #test/ChainedOperatorTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/ChangeBinOffsetTest.h
		 #test/CheckWorkspacesMatchTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/CloneWorkspaceTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/CommutativeBinaryOperationTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/CompressedRebinTest.h
		 test/ConjoinWorkspacesTest.h
		 #test/ConvertFromDistributionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/ConvertSpectrumAxisTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/ConvertToDistributionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/ConvertToHistogramTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/ConvertToMatrixWorkspaceTest.h
		 #test/ConvertToPointDataTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/ConvertUnitsTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/CorrectKiKfTest.h
		 #test/CorrectToFileTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/CreateCalFileByNamesTest.h
		 test/CreateDummyCalFileTest.h
		 test/CreateSingleValuedWorkspaceTest.h
		 test/CreateWorkspaceTest.h
		 #test/CropWorkspaceTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/CuboidGaugeVolumeAbsorptionTest.h # TODO uses ComponentCreationHelpers.hh AND WorkspaceCreationHelper.hh
		 #test/CylinderAbsorptionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/DetectorEfficiencyCorTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/DetectorEfficiencyVariationTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/DiffractionEventReadDetCalTest.h
		 test/DiffractionFocussing2Test.h
		 test/DiffractionFocussingTest.h
		 #test/DivideTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/DspacemaptoCalTest.h
		 #test/ExponentialCorrectionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/ExponentialTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/ExtractFFTSpectrumTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/ExtractMaskingTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/ExtractSingleSpectrumTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/FFTSmooth2Test.h # TODO uses WorkspaceCreationHelper.hh
		 test/FFTTest.h
		 #test/FilterByLogValueTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/FilterByTimeTest.h
		 #test/FindCenterOfMassPositionTest.h # TODO uses ComponentCreationHelper.hh via SANSInstrumentCreationHelper.h
		 #test/FindDeadDetectorsTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/FindDetectorsOutsideLimitsTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/FindPeaksTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/FlatBackgroundTest.h
		 #test/FlatPlateAbsorptionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/GetDetectorOffsetsTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/GetEiTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/GhostCorrectionTest.h # TODO uses ComponentCreationHelpers.hh AND WorkspaceCreationHelper.hh
		 test/GroupWorkspacesTest.h
		 test/He3TubeEfficiencyTest.h
		 #test/HRPDSlabCanAbsorptionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/IntegrationTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/InterpolatingRebinTest.h
		 #test/LogarithmTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/MaskBinsTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/MedianDetectorTestTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/MemoryLeakTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/MergeRunsTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/MinusTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/MonteCarloAbsorptionTest.h # TODO uses ComponentCreationHelpers.hh AND WorkspaceCreationHelper.hh
		 #test/MultipleScatteringAbsorption.h # TODO uses ComponentCreationHelpers.hh AND WorkspaceCreationHelper.hh
		 test/MultiplyRangeTest.h
		 #test/MultiplyTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/MuonAlphaCalcTest.h
		 test/MuonAsymmetryCalcTest.h
		 test/MuonRemoveExpDecayTest.h
		 #test/NormaliseByCurrentTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/NormaliseToMonitorTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/OneMinusExponentialCorTest.h # TODO uses ComponentCreationHelpers.hh AND WorkspaceCreationHelper.hh
		 test/PlotAsymmetryByLogValueTest.h
		 #test/PlusTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/PointByPointVCorrectionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/PoissonErrorsTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/PolynomialCorrectionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/PowerLawCorrectionTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/PowerTest.h
		 test/Q1DTest.h
		 test/Q1DWeightedTest.h
		 test/QxyTest.h
		 test/RealFFTTest.h
		 test/RebinTest.h
		 #test/RebinToWorkspaceTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/RebunchTest.h
		 test/RegroupTest.h
		 test/RemoveBinsTest.h
		 #test/RenameWorkspaceTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/ReplaceSpecialValuesTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/SANSInstrumentCreationHelper.h # TODO uses WorkspaceCreationHelper.hh
		 test/SaveSNSNexusTest.h
		 #test/ScaleTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/SmoothDataTest.h
		 test/SofQWTest.h
		 test/SolidAngleCorrectionTest.h
		 #test/SolidAngleTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/SortTest.h
		 #test/StripPeaksTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/StripVanadiumPeaksTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/SumNeighboursTest.h
		 test/SumRowColumnTest.h
		 #test/SumSpectraTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/TofCorrectionTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/TransposeTest.h # TODO uses WorkspaceCreationHelper.hh
		 #test/UnaryOperationTest.h # TODO uses WorkspaceCreationHelper.hh
		 test/UnGroupWorkspacesTest.h
		 test/UnwrapTest.h
                 test/WeightedMeanTest.h
		 #test/WorkspaceGroupTest.h  # TODO uses WorkspaceCreationHelper.hh
		 )

# Add the target for this directory
add_library ( Algorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Algorithms PROPERTIES OUTPUT_NAME MantidAlgorithms )
# Add to the 'Framework' group in VS
set_property ( TARGET Algorithms PROPERTY FOLDER "Framework" )

include_directories ( inc )

target_link_libraries ( Algorithms ${MANTIDLIBS} ${GSL_LIBRARIES} )

if ( CXXTEST_FOUND )
  include_directories ( ../CurveFitting/inc ../DataHandling/inc ../Nexus/inc ../TestHelpers/inc )
  cxxtest_add_test ( AlgorithmsTest ${TEST_FILES} )
  target_link_libraries ( AlgorithmsTest Algorithms CurveFitting DataHandling Nexus TestHelpers )
  add_dependencies ( FrameworkTests AlgorithmsTest )
endif ()
