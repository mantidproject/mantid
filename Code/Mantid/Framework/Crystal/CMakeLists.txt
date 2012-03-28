set ( SRC_FILES
	src/AnvredCorrection.cpp
	src/CalculateUMatrix.cpp
	src/CentroidPeaks.cpp
	src/FindSXPeaks.cpp
	src/FindUBUsingFFT.cpp
	src/FindUBUsingIndexedPeaks.cpp
	src/FindUBUsingLatticeParameters.cpp
	src/FindUBUsingMinMaxD.cpp
	src/IndexPeaks.cpp
        src/ShowPossibleCells.cpp
        src/SelectCellOfType.cpp
        src/SelectCellWithForm.cpp
	src/IntegratePeakTimeSlices.cpp
	src/LoadHKL.cpp
	src/LoadIsawPeaks.cpp
    src/IndexSXPeaks.cpp
	src/LoadIsawUB.cpp
	src/MaskPeaksWorkspace.cpp
	src/NormaliseVanadium.cpp
	src/OptimizeExtinctionParameters.cpp
	src/PeakIntegration.cpp
	src/PeakIntensityVsRadius.cpp
	src/PredictPeaks.cpp
	src/SaveHKL.cpp
	src/SaveIsawPeaks.cpp
	src/SaveIsawUB.cpp
	src/SavePeaksFile.cpp
	src/SCDCalibratePanels.cpp
	src/SetGoniometer.cpp
	src/SetUB.cpp
	src/SortHKL.cpp
	src/TOFExtinction.cpp
)

set ( SRC_UNITY_IGNORE_FILES )
set ( INC_FILES
	inc/MantidCrystal/AnvredCorrection.h
	inc/MantidCrystal/CalculateUMatrix.h
	inc/MantidCrystal/CentroidPeaks.h
	inc/MantidCrystal/FindSXPeaks.h
	inc/MantidCrystal/FindUBUsingFFT.h
	inc/MantidCrystal/FindUBUsingIndexedPeaks.h
	inc/MantidCrystal/FindUBUsingLatticeParameters.h
	inc/MantidCrystal/FindUBUsingMinMaxD.h
	inc/MantidCrystal/IndexPeaks.h
	inc/MantidCrystal/ShowPossibleCells.h
        inc/MantidCrystal/SelectCellOfType.h
        inc/MantidCrystal/SelectCellWithForm.h
    inc/MantidCrystal/IndexSXPeaks.h
        inc/MantidCrystal/GSLFunctions.h
	inc/MantidCrystal/IntegratePeakTimeSlices.h
	inc/MantidCrystal/LoadHKL.h
	inc/MantidCrystal/LoadIsawPeaks.h
	inc/MantidCrystal/LoadIsawUB.h
	inc/MantidCrystal/MaskPeaksWorkspace.h
	inc/MantidCrystal/NormaliseVanadium.h
	inc/MantidCrystal/OptimizeExtinctionParameters.h
	inc/MantidCrystal/PeakIntegration.h
	inc/MantidCrystal/PeakIntensityVsRadius.h
	inc/MantidCrystal/PredictPeaks.h
	inc/MantidCrystal/SaveHKL.h
	inc/MantidCrystal/SaveIsawPeaks.h
	inc/MantidCrystal/SaveIsawUB.h
	inc/MantidCrystal/SavePeaksFile.h
	inc/MantidCrystal/SCDCalibratePanels.h
	inc/MantidCrystal/SetGoniometer.h
	inc/MantidCrystal/SetUB.h
	inc/MantidCrystal/SortHKL.h
	inc/MantidCrystal/TOFExtinction.h
)

set ( TEST_FILES
	test/AnvredCorrectionTest.h
	test/CalculateUMatrixTest.h
	test/CentroidPeaksTest.h
	test/FindSXPeaksTest.h
	test/FindUBUsingFFTTest.h
	test/FindUBUsingIndexedPeaksTest.h
	test/FindUBUsingLatticeParametersTest.h
	test/FindUBUsingMinMaxDTest.h
	test/IndexPeaksTest.h
        test/ShowPossibleCellsTest.h
        test/SelectCellOfTypeTest.h
        test/SelectCellWithFormTest.h
    test/IndexSXPeaksTest.h
	test/IntegratePeakTimeSlicesTest.h
	test/LoadHKLTest.h
	test/LoadIsawPeaksTest.h
	test/LoadIsawUBTest.h
	test/MaskPeaksWorkspaceTest.h
	test/NormaliseVanadiumTest.h
	test/PeakIntegrationTest.h
	test/PeakIntensityVsRadiusTest.h
	test/PredictPeaksTest.h
	test/SaveHKLTest.h
	test/SaveIsawPeaksTest.h
	test/SaveIsawUBTest.h
	test/SavePeaksFileTest.h
	test/SetGoniometerTest.h
	test/SetUBTest.h
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
#include_directories ( ../MDEvents/inc)

target_link_libraries ( Crystal ${MANTIDLIBS} ${GSL_LIBRARIES} )

if ( CXXTEST_FOUND )
  include_directories ( ../DataHandling/inc ../MDEvents/inc ../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../TestHelpers/src/ComponentCreationHelper.cpp 
                        ../TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../TestHelpers/src/MDEventsTestHelper.cpp )
  cxxtest_add_test ( CrystalTest ${TEST_FILES} )
  target_link_libraries ( CrystalTest Crystal DataHandling MDEvents )
  add_dependencies ( FrameworkTests CrystalTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET CrystalTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Crystal DESTINATION ${PLUGINS_DIR} )
