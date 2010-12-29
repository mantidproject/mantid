set ( SRC_FILES src/AbsorptionCorrection.cpp src/AddSampleLog.cpp
      src/AlignDetectors.cpp src/AnyShapeAbsorption.cpp
      src/ApplyTransmissionCorrection.cpp src/BinaryOperation.cpp
      src/CalculateEfficiency.cpp
      src/CalculateTransmissionBeamSpreader.cpp
      src/CalculateTransmission.cpp src/ChangeBinOffset.cpp
      src/CheckWorkspacesMatch.cpp src/CloneWorkspace.cpp
      src/CommutativeBinaryOperation.cpp src/ConjoinWorkspaces.cpp
      src/ConvertFromDistribution.cpp src/ConvertSpectrumAxis.cpp
      src/ConvertToDistribution.cpp src/ConvertUnits.cpp
      src/CorrectKiKf.cpp src/CorrectToFile.cpp
      src/CreateCalFileByNames.cpp src/CreateSingleValuedWorkspace.cpp
      src/CreateWorkspace.cpp src/CropWorkspace.cpp
      src/CrossCorrelate.cpp src/CuboidGaugeVolumeAbsorption.cpp
      src/CylinderAbsorption.cpp src/DetectorEfficiencyCor.cpp
      src/DetectorEfficiencyVariation.cpp
      src/DiffractionFocussing2.cpp src/DiffractionFocussing.cpp
      src/Divide.cpp src/ElasticWindow.cpp
      src/ExponentialCorrection.cpp src/Exponential.cpp
      src/ExtractFFTSpectrum.cpp src/ExtractSingleSpectrum.cpp
      src/FFT.cpp src/FFTSmooth2.cpp src/FFTSmooth.cpp
      src/FilterByLogValue.cpp src/FilterByTime.cpp
      src/FindCenterOfMassPosition.cpp src/FindDeadDetectors.cpp
      src/FindDetectorsOutsideLimits.cpp src/FindPeaks.cpp
      src/FlatBackground.cpp src/FlatPlateAbsorption.cpp
      src/GeneralisedSecondDifference.cpp src/GetDetectorOffsets.cpp
      src/GetEi2.cpp src/GetEi.cpp src/GhostCorrection.cpp
      src/GroupWorkspaces.cpp src/He3TubeEfficiency.cpp
      src/HRPDSlabCanAbsorption.cpp src/InputWSDetectorInfo.cpp
      src/InterpolatingRebin.cpp src/Logarithm.cpp src/MaskBins.cpp
      src/MaskDetectorsIf.cpp src/Max.cpp src/MedianDetectorTest.cpp
      src/MergeRuns.cpp src/Minus.cpp
      src/MultipleScatteringAbsorption.cpp src/Multiply.cpp
      src/MultiplyRange.cpp src/MuonAlphaCalc.cpp
      src/MuonAsymmetryCalc.cpp src/MuonRemoveExpDecay.cpp
      src/NormaliseByCurrent.cpp src/NormaliseToMonitor.cpp
      src/OneMinusExponentialCor.cpp src/PlotAsymmetryByLogValue.cpp
      src/Plus.cpp src/PointByPointVCorrection.cpp
      src/PoissonErrors.cpp src/PolynomialCorrection.cpp src/Power.cpp
      src/PowerLawCorrection.cpp src/Q1D.cpp src/Q1DWeighted.cpp
      src/Qxy.cpp src/ReadGroupsFromFile.cpp src/RealFFT.cpp
      src/Rebin.cpp src/RebinToWorkspace.cpp src/Rebunch.cpp
      src/Regroup.cpp src/RemoveBins.cpp src/RenameWorkspace.cpp
      src/ReplaceSpecialValues.cpp src/Scale.cpp
      src/SimpleIntegration.cpp src/SmoothData.cpp src/SofQW.cpp
      src/SolidAngleCorrection.cpp src/SolidAngle.cpp src/Sort.cpp
      src/StripPeaks.cpp src/StripVanadiumPeaks.cpp
      src/SumNeighbours.cpp src/SumRowColumn.cpp src/SumSpectra.cpp
      src/TofCorrection.cpp src/Transpose.cpp src/UnaryOperation.cpp
      src/UnGroupWorkspaces.cpp src/Unwrap.cpp src/WeightedMean.cpp )

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
      inc/MantidAlgorithms/ConvertUnits.h
      inc/MantidAlgorithms/CorrectKiKf.h
      inc/MantidAlgorithms/CorrectToFile.h
      inc/MantidAlgorithms/CreateCalFileByNames.h
      inc/MantidAlgorithms/CreateSingleValuedWorkspace.h
      inc/MantidAlgorithms/CreateWorkspace.h
      inc/MantidAlgorithms/CropWorkspace.h
      inc/MantidAlgorithms/CrossCorrelate.h
      inc/MantidAlgorithms/CuboidGaugeVolumeAbsorption.h
      inc/MantidAlgorithms/CylinderAbsorption.h
      inc/MantidAlgorithms/DetectorEfficiencyCor.h
      inc/MantidAlgorithms/DetectorEfficiencyVariation.h
      inc/MantidAlgorithms/DiffractionFocussing2.h
      inc/MantidAlgorithms/DiffractionFocussing.h
      inc/MantidAlgorithms/Divide.h
      inc/MantidAlgorithms/ElasticWindow.h
      inc/MantidAlgorithms/ExponentialCorrection.h
      inc/MantidAlgorithms/Exponential.h
      inc/MantidAlgorithms/ExtractFFTSpectrum.h
      inc/MantidAlgorithms/ExtractSingleSpectrum.h
      inc/MantidAlgorithms/FFT.h inc/MantidAlgorithms/FFTSmooth2.h
      inc/MantidAlgorithms/FFTSmooth.h
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
      inc/MantidAlgorithms/GetEi2.h inc/MantidAlgorithms/GetEi.h
      inc/MantidAlgorithms/GhostCorrection.h
      inc/MantidAlgorithms/GroupWorkspaces.h
      inc/MantidAlgorithms/He3TubeEfficiency.h
      inc/MantidAlgorithms/HRPDSlabCanAbsorption.h
      inc/MantidAlgorithms/InputWSDetectorInfo.h
      inc/MantidAlgorithms/InterpolatingRebin.h
      inc/MantidAlgorithms/Logarithm.h inc/MantidAlgorithms/MaskBins.h
      inc/MantidAlgorithms/MaskDetectorsIf.h
      inc/MantidAlgorithms/Max.h
      inc/MantidAlgorithms/MedianDetectorTest.h
      inc/MantidAlgorithms/MergeRuns.h inc/MantidAlgorithms/Minus.h
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
      inc/MantidAlgorithms/Q1D.h inc/MantidAlgorithms/Q1DWeighted.h
      inc/MantidAlgorithms/Qxy.h
      inc/MantidAlgorithms/ReadGroupsFromFile.h
      inc/MantidAlgorithms/RealFFT.h inc/MantidAlgorithms/Rebin.h
      inc/MantidAlgorithms/RebinToWorkspace.h
      inc/MantidAlgorithms/Rebunch.h inc/MantidAlgorithms/Regroup.h
      inc/MantidAlgorithms/RemoveBins.h
      inc/MantidAlgorithms/RenameWorkspace.h
      inc/MantidAlgorithms/ReplaceSpecialValues.h
      inc/MantidAlgorithms/Scale.h
      inc/MantidAlgorithms/SimpleIntegration.h
      inc/MantidAlgorithms/SmoothData.h inc/MantidAlgorithms/SofQW.h
      inc/MantidAlgorithms/SolidAngleCorrection.h
      inc/MantidAlgorithms/SolidAngle.h inc/MantidAlgorithms/Sort.h
      inc/MantidAlgorithms/StripPeaks.h
      inc/MantidAlgorithms/StripVanadiumPeaks.h
      inc/MantidAlgorithms/SumNeighbours.h
      inc/MantidAlgorithms/SumRowColumn.h
      inc/MantidAlgorithms/SumSpectra.h
      inc/MantidAlgorithms/TofCorrection.h
      inc/MantidAlgorithms/Transpose.h
      inc/MantidAlgorithms/UnaryOperation.h
      inc/MantidAlgorithms/UnGroupWorkspaces.h
      inc/MantidAlgorithms/Unwrap.h
      inc/MantidAlgorithms/WeightedMean.h)

# Add the target for this directory
add_library ( Algorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Algorithms PROPERTIES OUTPUT_NAME MantidAlgorithms )
# Add to the 'Framework' group in VS
set_property ( TARGET Algorithms PROPERTY FOLDER "Framework" )

include_directories ( inc )

target_link_libraries ( Algorithms ${MANTIDLIBS} ${GSL_LIBRARIES} )