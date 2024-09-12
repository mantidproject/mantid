set(SRC_FILES
    src/AddPeakHKL.cpp
    src/AnvredCorrection.cpp
    src/CalculatePeaksHKL.cpp
    src/CalculateUMatrix.cpp
    src/CalibrationHelpers.cpp
    src/CentroidPeaks.cpp
    src/ClearUB.cpp
    src/Cluster.cpp
    src/ClusterRegister.cpp
    src/CombinePeaksWorkspaces.cpp
    src/CompositeCluster.cpp
    src/ConnectedComponentLabeling.cpp
    src/ConvertPeaksWorkspace.cpp
    src/CountReflections.cpp
    src/DiffPeaksWorkspaces.cpp
    src/DisjointElement.cpp
    src/FilterPeaks.cpp
    src/FindClusterFaces.cpp
    src/FindGoniometerAngles.cpp
    src/FindSXPeaks.cpp
    src/FindSXPeaksHelper.cpp
    src/FindUBUsingFFT.cpp
    src/FindUBUsingIndexedPeaks.cpp
    src/FindUBUsingLatticeParameters.cpp
    src/GoniometerAnglesFromPhiRotation.cpp
    src/HardThresholdBackground.cpp
    src/HasUB.cpp
    src/IndexPeaks.cpp
    src/IndexSXPeaks.cpp
    src/IntegratePeakTimeSlices.cpp
    src/IntegratePeaksHybrid.cpp
    src/IntegratePeaksUsingClusters.cpp
    src/LoadHKL.cpp
    src/LoadIsawPeaks.cpp
    src/LoadIsawSpectrum.cpp
    src/LoadIsawUB.cpp
    src/MaskPeaksWorkspace.cpp
    src/NormaliseVanadium.cpp
    src/OptimizeCrystalPlacement.cpp
    src/OptimizeLatticeForCellType.cpp
    src/PeakAlgorithmHelpers.cpp
    src/PeakBackground.cpp
    src/PeakClusterProjection.cpp
    src/PeakHKLErrors.cpp
    src/PeakIntegration.cpp
    src/PeakIntensityVsRadius.cpp
    src/PeakStatisticsTools.cpp
    src/PeaksInRegion.cpp
    src/PeaksIntersection.cpp
    src/PeaksOnSurface.cpp
    src/PredictFractionalPeaks.cpp
    src/PredictPeaks.cpp
    src/PredictSatellitePeaks.cpp
    src/SCDCalibratePanels.cpp
    src/SCDCalibratePanels2.cpp
    src/SCDCalibratePanels2ObjFunc.cpp
    src/SCDPanelErrors.cpp
    src/SaveHKL.cpp
    src/SaveIsawPeaks.cpp
    src/SaveIsawUB.cpp
    src/SaveLauenorm.cpp
    src/SelectCellOfType.cpp
    src/SelectCellWithForm.cpp
    src/SetCrystalLocation.cpp
    src/SetGoniometer.cpp
    src/SetSpecialCoordinates.cpp
    src/SetUB.cpp
    src/ShowPeakHKLOffsets.cpp
    src/ShowPossibleCells.cpp
    src/SortHKL.cpp
    src/SortPeaksWorkspace.cpp
    src/StatisticsOfPeaksWorkspace.cpp
    src/TransformHKL.cpp
    src/RotateSampleShape.cpp
)

set(INC_FILES
    inc/MantidCrystal/AddPeakHKL.h
    inc/MantidCrystal/AnvredCorrection.h
    inc/MantidCrystal/BackgroundStrategy.h
    inc/MantidCrystal/CalculatePeaksHKL.h
    inc/MantidCrystal/CalculateUMatrix.h
    inc/MantidCrystal/CalibrationHelpers.h
    inc/MantidCrystal/CentroidPeaks.h
    inc/MantidCrystal/ClearUB.h
    inc/MantidCrystal/Cluster.h
    inc/MantidCrystal/ClusterRegister.h
    inc/MantidCrystal/CombinePeaksWorkspaces.h
    inc/MantidCrystal/CompositeCluster.h
    inc/MantidCrystal/ConnectedComponentLabeling.h
    inc/MantidCrystal/ConvertPeaksWorkspace.h
    inc/MantidCrystal/CountReflections.h
    inc/MantidCrystal/DiffPeaksWorkspaces.h
    inc/MantidCrystal/DisjointElement.h
    inc/MantidCrystal/FilterPeaks.h
    inc/MantidCrystal/FindClusterFaces.h
    inc/MantidCrystal/FindGoniometerAngles.h
    inc/MantidCrystal/FindSXPeaks.h
    inc/MantidCrystal/FindSXPeaksHelper.h
    inc/MantidCrystal/FindUBUsingFFT.h
    inc/MantidCrystal/FindUBUsingIndexedPeaks.h
    inc/MantidCrystal/FindUBUsingLatticeParameters.h
    inc/MantidCrystal/GSLFunctions.h
    inc/MantidCrystal/GoniometerAnglesFromPhiRotation.h
    inc/MantidCrystal/HardThresholdBackground.h
    inc/MantidCrystal/HasUB.h
    inc/MantidCrystal/ICluster.h
    inc/MantidCrystal/IndexPeaks.h
    inc/MantidCrystal/IndexSXPeaks.h
    inc/MantidCrystal/IntegratePeakTimeSlices.h
    inc/MantidCrystal/IntegratePeaksHybrid.h
    inc/MantidCrystal/IntegratePeaksUsingClusters.h
    inc/MantidCrystal/LoadHKL.h
    inc/MantidCrystal/LoadIsawPeaks.h
    inc/MantidCrystal/LoadIsawSpectrum.h
    inc/MantidCrystal/LoadIsawUB.h
    inc/MantidCrystal/MaskPeaksWorkspace.h
    inc/MantidCrystal/NormaliseVanadium.h
    inc/MantidCrystal/OptimizeCrystalPlacement.h
    inc/MantidCrystal/OptimizeLatticeForCellType.h
    inc/MantidCrystal/PeakAlgorithmHelpers.h
    inc/MantidCrystal/PeakBackground.h
    inc/MantidCrystal/PeakClusterProjection.h
    inc/MantidCrystal/PeakHKLErrors.h
    inc/MantidCrystal/PeakIntegration.h
    inc/MantidCrystal/PeakIntensityVsRadius.h
    inc/MantidCrystal/PeakStatisticsTools.h
    inc/MantidCrystal/PeaksInRegion.h
    inc/MantidCrystal/PeaksIntersection.h
    inc/MantidCrystal/PeaksOnSurface.h
    inc/MantidCrystal/PredictFractionalPeaks.h
    inc/MantidCrystal/PredictPeaks.h
    inc/MantidCrystal/PredictSatellitePeaks.h
    inc/MantidCrystal/SCDCalibratePanels.h
    inc/MantidCrystal/SCDCalibratePanels2.h
    inc/MantidCrystal/SCDCalibratePanels2ObjFunc.h
    inc/MantidCrystal/SCDPanelErrors.h
    inc/MantidCrystal/SaveHKL.h
    inc/MantidCrystal/SaveIsawPeaks.h
    inc/MantidCrystal/SaveIsawUB.h
    inc/MantidCrystal/SaveLauenorm.h
    inc/MantidCrystal/SelectCellOfType.h
    inc/MantidCrystal/SelectCellWithForm.h
    inc/MantidCrystal/SetCrystalLocation.h
    inc/MantidCrystal/SetGoniometer.h
    inc/MantidCrystal/SetSpecialCoordinates.h
    inc/MantidCrystal/SetUB.h
    inc/MantidCrystal/ShowPeakHKLOffsets.h
    inc/MantidCrystal/ShowPossibleCells.h
    inc/MantidCrystal/SortHKL.h
    inc/MantidCrystal/SortPeaksWorkspace.h
    inc/MantidCrystal/StatisticsOfPeaksWorkspace.h
    inc/MantidCrystal/TransformHKL.h
    inc/MantidCrystal/RotateSampleShape.h
)

set(TEST_FILES
    AddPeakHKLTest.h
    AnvredCorrectionTest.h
    CalculatePeaksHKLTest.h
    CalculateUMatrixTest.h
    CalibrationHelpersTest.h
    CentroidPeaksTest.h
    ClearUBTest.h
    ClusterRegisterTest.h
    ClusterTest.h
    CombinePeaksWorkspacesTest.h
    CompositeClusterTest.h
    ConnectedComponentLabelingTest.h
    ConvertPeaksWorkspaceTest.h
    DiffPeaksWorkspacesTest.h
    DisjointElementTest.h
    FilterPeaksTest.h
    FindClusterFacesTest.h
    FindGoniometerAnglesTest.h
    FindSXPeaksHelperTest.h
    FindSXPeaksTest.h
    FindUBUsingFFTTest.h
    FindUBUsingIndexedPeaksTest.h
    FindUBUsingLatticeParametersTest.h
    GoniometerAnglesFromPhiRotationTest.h
    HardThresholdBackgroundTest.h
    HasUBTest.h
    IndexPeaksTest.h
    IndexSXPeaksTest.h
    IntegratePeakTimeSlicesTest.h
    IntegratePeaksHybridTest.h
    IntegratePeaksUsingClustersTest.h
    LoadHKLTest.h
    LoadIsawPeaksTest.h
    LoadIsawSpectrumTest.h
    LoadIsawUBTest.h
    MaskPeaksWorkspaceTest.h
    NormaliseVanadiumTest.h
    OptimizeCrystalPlacementTest.h
    OptimizeLatticeForCellTypeTest.h
    PeakBackgroundTest.h
    PeakClusterProjectionTest.h
    PeakHKLErrorsTest.h
    PeakIntegrationTest.h
    PeakIntensityVsRadiusTest.h
    PeakStatisticsToolsTest.h
    PeaksInRegionTest.h
    PeaksOnSurfaceTest.h
    PredictFractionalPeaksTest.h
    PredictPeaksTest.h
    PredictSatellitePeaksTest.h
    SCDCalibratePanels2Test.h
    SCDCalibratePanels2ObjFuncTest.h
    SCDCalibratePanelsTest.h
    SaveHKLTest.h
    SaveIsawPeaksTest.h
    SaveIsawUBTest.h
    SaveLauenormTest.h
    SelectCellOfTypeTest.h
    SelectCellWithFormTest.h
    SetCrystalLocationTest.h
    SetGoniometerTest.h
    SetSpecialCoordinatesTest.h
    SetUBTest.h
    ShowPeakHKLOffsetsTest.h
    ShowPossibleCellsTest.h
    SortHKLTest.h
    SortPeaksWorkspaceTest.h
    StatisticsOfPeaksWorkspaceTest.h
    TransformHKLTest.h
    RotateSampleShapeTest.h
)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add a precompiled header where they are supported
enable_precompiled_headers(inc/MantidCrystal/PrecompiledHeader.h SRC_FILES)

# Add the target for this directory
add_library(Crystal ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Crystal ALIAS Crystal)

target_include_directories(
  Crystal PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(Crystal PROPERTIES OUTPUT_NAME MantidCrystal)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Crystal PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS/")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Crystal PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Crystal PROPERTY FOLDER "MantidFramework")

include_directories(inc)

target_link_libraries(
  Crystal
  PUBLIC Mantid::API Mantid::Geometry Mantid::Kernel
  PRIVATE Mantid::DataObjects Mantid::Indexing Mantid::DataHandling
)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  Crystal PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/Crystal> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(Crystal FALSE)

# Installation settings
mtd_install_framework_lib(TARGETS Crystal PLUGIN_LIB)
