
set ( SRC_FILES
	src/AnvredCorrection.cpp
	src/CalculatePeaksHKL.cpp
	src/CalculateUMatrix.cpp
	src/CentroidPeaks.cpp
	src/CombinePeaksWorkspaces.cpp
	src/DiffPeaksWorkspaces.cpp
	src/FilterPeaks.cpp
	src/FindSXPeaks.cpp
	src/FindUBUsingFFT.cpp
	src/FindUBUsingIndexedPeaks.cpp
	src/FindUBUsingLatticeParameters.cpp
	src/FindUBUsingMinMaxD.cpp
	src/GoniometerAnglesFromPhiRotation.cpp
	src/IndexPeaks.cpp
	src/IndexSXPeaks.cpp
	src/IntegratePeakTimeSlices.cpp
	src/LoadHKL.cpp
	src/LoadIsawPeaks.cpp
	src/LoadIsawUB.cpp
	src/MaskPeaksWorkspace.cpp
	src/NormaliseVanadium.cpp
	src/OptimizeCrystalPlacement.cpp
	src/OptimizeExtinctionParameters.cpp
	src/PeakHKLErrors.cpp
	src/PeakIntegration.cpp
	src/PeakIntensityVsRadius.cpp
	src/PeaksInRegion.cpp
	src/PeaksIntersection.cpp
	src/PeaksOnSurface.cpp
	src/PredictFractionalPeaks.cpp
	src/PredictPeaks.cpp
	src/SCDCalibratePanels.cpp
	src/SCDPanelErrors.cpp
	src/SaveHKL.cpp
	src/SaveIsawPeaks.cpp
	src/SaveIsawUB.cpp
	src/SavePeaksFile.cpp
	src/SelectCellOfType.cpp
	src/SelectCellWithForm.cpp
	src/SetGoniometer.cpp
	src/SetSpecialCoordinates.cpp
	src/SetUB.cpp
	src/ShowPeakHKLOffsets.cpp
	src/ShowPossibleCells.cpp
	src/SortHKL.cpp
	src/SortPeaksWorkspace.cpp
	src/TOFExtinction.cpp
	src/TransformHKL.cpp
)

set ( SRC_UNITY_IGNORE_FILES )
set ( INC_FILES
	inc/MantidCrystal/AnvredCorrection.h
	inc/MantidCrystal/CalculatePeaksHKL.h
	inc/MantidCrystal/CalculateUMatrix.h
	inc/MantidCrystal/CentroidPeaks.h
	inc/MantidCrystal/CombinePeaksWorkspaces.h
	inc/MantidCrystal/DiffPeaksWorkspaces.h
	inc/MantidCrystal/FilterPeaks.h
	inc/MantidCrystal/FindSXPeaks.h
	inc/MantidCrystal/FindUBUsingFFT.h
	inc/MantidCrystal/FindUBUsingIndexedPeaks.h
	inc/MantidCrystal/FindUBUsingLatticeParameters.h
	inc/MantidCrystal/FindUBUsingMinMaxD.h
	inc/MantidCrystal/GSLFunctions.h
	inc/MantidCrystal/GoniometerAnglesFromPhiRotation.h
	inc/MantidCrystal/IndexPeaks.h
	inc/MantidCrystal/IndexSXPeaks.h
	inc/MantidCrystal/IntegratePeakTimeSlices.h
	inc/MantidCrystal/LoadHKL.h
	inc/MantidCrystal/LoadIsawPeaks.h
	inc/MantidCrystal/LoadIsawUB.h
	inc/MantidCrystal/MaskPeaksWorkspace.h
	inc/MantidCrystal/NormaliseVanadium.h
	inc/MantidCrystal/OptimizeCrystalPlacement.h
	inc/MantidCrystal/OptimizeExtinctionParameters.h
	inc/MantidCrystal/PeakHKLErrors.h
	inc/MantidCrystal/PeakIntegration.h
	inc/MantidCrystal/PeakIntensityVsRadius.h
	inc/MantidCrystal/PeaksInRegion.h
	inc/MantidCrystal/PeaksIntersection.h
	inc/MantidCrystal/PeaksOnSurface.h
	inc/MantidCrystal/PredictFractionalPeaks.h
	inc/MantidCrystal/PredictPeaks.h
	inc/MantidCrystal/SCDCalibratePanels.h
	inc/MantidCrystal/SCDPanelErrors.h
	inc/MantidCrystal/SaveHKL.h
	inc/MantidCrystal/SaveIsawPeaks.h
	inc/MantidCrystal/SaveIsawUB.h
	inc/MantidCrystal/SavePeaksFile.h
	inc/MantidCrystal/SelectCellOfType.h
	inc/MantidCrystal/SelectCellWithForm.h
	inc/MantidCrystal/SetGoniometer.h
	inc/MantidCrystal/SetSpecialCoordinates.h
	inc/MantidCrystal/SetUB.h
	inc/MantidCrystal/ShowPeakHKLOffsets.h
	inc/MantidCrystal/ShowPossibleCells.h
	inc/MantidCrystal/SortHKL.h
	inc/MantidCrystal/SortPeaksWorkspace.h
	inc/MantidCrystal/TOFExtinction.h
	inc/MantidCrystal/TransformHKL.h
)

set ( TEST_FILES
	AnvredCorrectionTest.h
	CalculatePeaksHKLTest.h
	CalculateUMatrixTest.h
	CentroidPeaksTest.h
	CombinePeaksWorkspacesTest.h
	DiffPeaksWorkspacesTest.h
	FilterPeaksTest.h
	FindSXPeaksTest.h
	FindUBUsingFFTTest.h
	FindUBUsingIndexedPeaksTest.h
	FindUBUsingLatticeParametersTest.h
	FindUBUsingMinMaxDTest.h
	GoniometerAnglesFromPhiRotationTest.h
	IndexPeaksTest.h
	IndexSXPeaksTest.h
	IntegratePeakTimeSlicesTest.h
	LoadHKLTest.h
	LoadIsawPeaksTest.h
	LoadIsawUBTest.h
	MaskPeaksWorkspaceTest.h
	NormaliseVanadiumTest.h
	OptimizeCrystalPlacementTest.h
	PeakHKLErrorsTest.h
	PeakIntegrationTest.h
	PeakIntensityVsRadiusTest.h
	PeaksInRegionTest.h
	PeaksOnSurfaceTest.h
	PredictFractionalPeaksTest.h
	PredictPeaksTest.h
	SCDCalibratePanelsTest.h
	SCDPanelErrorsTest.h
	SaveHKLTest.h
	SaveIsawPeaksTest.h
	SaveIsawUBTest.h
	SavePeaksFileTest.h
	SelectCellOfTypeTest.h
	SelectCellWithFormTest.h
	SetGoniometerTest.h
	SetSpecialCoordinatesTest.h
	SetUBTest.h
	ShowPeakHKLOffsetsTest.h
	ShowPossibleCellsTest.h
	SortHKLTest.h
	SortPeaksWorkspaceTest.h
	TransformHKLTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Crystal SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( Crystal ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Crystal PROPERTIES OUTPUT_NAME MantidCrystal )
# Add to the 'Framework' group in VS
set_property ( TARGET Crystal PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( Crystal ${MANTIDLIBS} ${GSL_LIBRARIES} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Crystal ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
