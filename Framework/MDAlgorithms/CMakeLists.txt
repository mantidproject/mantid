# GLOBs should be replaced with explicit listings
set(SRC_FILES
    src/AccumulateMD.cpp
    src/AndMD.cpp
    src/ApplyDetailedBalanceMD.cpp
    src/BaseConvertToDiffractionMDWorkspace.cpp
    src/BinMD.cpp
    src/BinaryOperationMD.cpp
    src/BooleanBinaryOperationMD.cpp
    src/CalculateCoverageDGS.cpp
    src/CentroidPeaksMD2.cpp
    src/ChangeQConvention.cpp
    src/CloneMDWorkspace.cpp
    src/CompactMD.cpp
    src/CompareMDWorkspaces.cpp
    src/ConvToMDBase.cpp
    src/ConvToMDEventsWS.cpp
    src/ConvToMDEventsWSIndexing.cpp
    src/ConvToMDHistoWS.cpp
    src/ConvToMDSelector.cpp
    src/ConvertCWPDMDToSpectra.cpp
    src/ConvertCWSDExpToMomentum.cpp
    src/ConvertCWSDMDtoHKL.cpp
    src/ConvertHFIRSCDtoMDE.cpp
    src/ConvertMDHistoToMatrixWorkspace.cpp
    src/ConvertSpiceDataToRealSpace.cpp
    src/ConvertToDetectorFaceMD.cpp
    src/ConvertToDiffractionMDWorkspace.cpp
    src/ConvertToDiffractionMDWorkspace2.cpp
    src/ConvertToDiffractionMDWorkspace3.cpp
    src/ConvertToMD.cpp
    src/ConvertToMDMinMaxGlobal.cpp
    src/ConvertToMDMinMaxLocal.cpp
    src/ConvertToMDParent.cpp
    src/CreateMD.cpp
    src/CreateMDHistoWorkspace.cpp
    src/CreateMDWorkspace.cpp
    src/CutMD.cpp
    src/DgsScatteredTransmissionCorrectionMD.cpp
    src/DisplayNormalizationSetter.cpp
    src/DivideMD.cpp
    src/EqualToMD.cpp
    src/EvaluateMDFunction.cpp
    src/ExponentialMD.cpp
    src/FakeMDEventData.cpp
    src/FindPeaksMD.cpp
    src/FitMD.cpp
    src/FlippingRatioCorrectionMD.cpp
    src/GetSpiceDataRawCountsFromMD.cpp
    src/GreaterThanMD.cpp
    src/IDynamicRebinning.cpp
    src/ImportMDEventWorkspace.cpp
    src/ImportMDHistoWorkspace.cpp
    src/ImportMDHistoWorkspaceBase.cpp
    src/Integrate3DEvents.cpp
    src/IntegrateEllipsoids.cpp
    src/IntegrateEllipsoidsTwoStep.cpp
    src/IntegrateEllipsoidsV1.cpp
    src/IntegrateEllipsoidsV2.cpp
    src/IntegrateFlux.cpp
    src/IntegrateMDHistoWorkspace.cpp
    src/IntegratePeaksCWSD.cpp
    src/IntegratePeaksMD.cpp
    src/IntegratePeaksMD2.cpp
    src/IntegratePeaksMDHKL.cpp
    src/IntegrateQLabEvents.cpp
    src/InvalidParameter.cpp
    src/InvalidParameterParser.cpp
    src/LessThanMD.cpp
    src/LoadDNSSCD.cpp
    src/LoadGaussCube.cpp
    src/LoadMD.cpp
    src/LoadSQW.cpp
    src/LoadSQW2.cpp
    src/LogarithmMD.cpp
    src/MDEventWSWrapper.cpp
    src/MDNorm.cpp
    src/MDNormDirectSC.cpp
    src/MDNormSCD.cpp
    src/MDTransfAxisNames.cpp
    src/MDTransfFactory.cpp
    src/MDTransfModQ.cpp
    src/MDTransfNoQ.cpp
    src/MDTransfQ3D.cpp
    src/MDWSDescription.cpp
    src/MDWSTransform.cpp
    src/MagneticFormFactorCorrectionMD.cpp
    src/DebyeWallerFactorCorrectionMD.cpp
    src/MaskMD.cpp
    src/MergeMD.cpp
    src/MergeMDFiles.cpp
    src/MinusMD.cpp
    src/MultiplyMD.cpp
    src/NotMD.cpp
    src/OrMD.cpp
    src/PlusMD.cpp
    src/PolarizationAngleCorrectionMD.cpp
    src/PowerMD.cpp
    src/PreprocessDetectorsToMD.cpp
    src/QTransform.cpp
    src/QueryMDWorkspace.cpp
    src/RecalculateTrajectoriesExtents.cpp
    src/ReplicateMD.cpp
    src/SaveIsawQvector.cpp
    src/SaveMD.cpp
    src/SaveMD2.cpp
    src/SaveZODS.cpp
    src/SetMDFrame.cpp
    src/SetMDUsingMask.cpp
    src/SliceMD.cpp
    src/SlicingAlgorithm.cpp
    src/SmoothMD.cpp
    src/ThresholdMD.cpp
    src/TransformMD.cpp
    src/TransposeMD.cpp
    src/UnaryOperationMD.cpp
    src/UnitsConversionHelper.cpp
    src/UserFunctionMD.cpp
    src/WeightedMeanMD.cpp
    src/XorMD.cpp
)

set(INC_FILES
    inc/MantidMDAlgorithms/AccumulateMD.h
    inc/MantidMDAlgorithms/AndMD.h
    inc/MantidMDAlgorithms/ApplyDetailedBalanceMD.h
    inc/MantidMDAlgorithms/BaseConvertToDiffractionMDWorkspace.h
    inc/MantidMDAlgorithms/BinMD.h
    inc/MantidMDAlgorithms/BinaryOperationMD.h
    inc/MantidMDAlgorithms/BooleanBinaryOperationMD.h
    inc/MantidMDAlgorithms/CalculateCoverageDGS.h
    inc/MantidMDAlgorithms/CentroidPeaksMD2.h
    inc/MantidMDAlgorithms/ChangeQConvention.h
    inc/MantidMDAlgorithms/CloneMDWorkspace.h
    inc/MantidMDAlgorithms/CompactMD.h
    inc/MantidMDAlgorithms/CompareMDWorkspaces.h
    inc/MantidMDAlgorithms/ConvToMDBase.h
    inc/MantidMDAlgorithms/ConvToMDEventsWSIndexing.h
    inc/MantidMDAlgorithms/ConvertCWPDMDToSpectra.h
    inc/MantidMDAlgorithms/ConvertCWSDExpToMomentum.h
    inc/MantidMDAlgorithms/ConvertCWSDMDtoHKL.h
    inc/MantidMDAlgorithms/ConvertHFIRSCDtoMDE.h
    inc/MantidMDAlgorithms/ConvertMDHistoToMatrixWorkspace.h
    inc/MantidMDAlgorithms/ConvertSpiceDataToRealSpace.h
    inc/MantidMDAlgorithms/ConvertToDetectorFaceMD.h
    inc/MantidMDAlgorithms/ConvertToDiffractionMDWorkspace.h
    inc/MantidMDAlgorithms/ConvertToDiffractionMDWorkspace2.h
    inc/MantidMDAlgorithms/ConvertToDiffractionMDWorkspace3.h
    inc/MantidMDAlgorithms/ConvertToMD.h
    inc/MantidMDAlgorithms/ConvertToMDMinMaxGlobal.h
    inc/MantidMDAlgorithms/ConvertToMDMinMaxLocal.h
    inc/MantidMDAlgorithms/ConvertToMDParent.h
    inc/MantidMDAlgorithms/CreateMD.h
    inc/MantidMDAlgorithms/CreateMDHistoWorkspace.h
    inc/MantidMDAlgorithms/CreateMDWorkspace.h
    inc/MantidMDAlgorithms/CutMD.h
    inc/MantidMDAlgorithms/DgsScatteredTransmissionCorrectionMD.h
    inc/MantidMDAlgorithms/DisplayNormalizationSetter.h
    inc/MantidMDAlgorithms/DivideMD.h
    inc/MantidMDAlgorithms/EqualToMD.h
    inc/MantidMDAlgorithms/EvaluateMDFunction.h
    inc/MantidMDAlgorithms/ExponentialMD.h
    inc/MantidMDAlgorithms/FakeMDEventData.h
    inc/MantidMDAlgorithms/FindPeaksMD.h
    inc/MantidMDAlgorithms/FitMD.h
    inc/MantidMDAlgorithms/FlippingRatioCorrectionMD.h
    inc/MantidMDAlgorithms/GSLFunctions.h
    inc/MantidMDAlgorithms/GetSpiceDataRawCountsFromMD.h
    inc/MantidMDAlgorithms/GreaterThanMD.h
    inc/MantidMDAlgorithms/IDynamicRebinning.h
    inc/MantidMDAlgorithms/ImportMDEventWorkspace.h
    inc/MantidMDAlgorithms/ImportMDHistoWorkspace.h
    inc/MantidMDAlgorithms/ImportMDHistoWorkspaceBase.h
    inc/MantidMDAlgorithms/Integrate3DEvents.h
    inc/MantidMDAlgorithms/IntegrateEllipsoids.h
    inc/MantidMDAlgorithms/IntegrateEllipsoidsTwoStep.h
    inc/MantidMDAlgorithms/IntegrateEllipsoidsV1.h
    inc/MantidMDAlgorithms/IntegrateEllipsoidsV2.h
    inc/MantidMDAlgorithms/IntegrateFlux.h
    inc/MantidMDAlgorithms/IntegrateMDHistoWorkspace.h
    inc/MantidMDAlgorithms/IntegratePeaksCWSD.h
    inc/MantidMDAlgorithms/IntegratePeaksMD.h
    inc/MantidMDAlgorithms/IntegratePeaksMD2.h
    inc/MantidMDAlgorithms/IntegratePeaksMDHKL.h
    inc/MantidMDAlgorithms/IntegrateQLabEvents.h
    inc/MantidMDAlgorithms/InvalidParameter.h
    inc/MantidMDAlgorithms/InvalidParameterParser.h
    inc/MantidMDAlgorithms/LessThanMD.h
    inc/MantidMDAlgorithms/LoadDNSSCD.h
    inc/MantidMDAlgorithms/LoadGaussCube.h
    inc/MantidMDAlgorithms/LoadMD.h
    inc/MantidMDAlgorithms/LoadSQW.h
    inc/MantidMDAlgorithms/LoadSQW2.h
    inc/MantidMDAlgorithms/LogarithmMD.h
    inc/MantidMDAlgorithms/MDBoxMaskFunction.h
    inc/MantidMDAlgorithms/MDEventTreeBuilder.h
    inc/MantidMDAlgorithms/MDEventWSWrapper.h
    inc/MantidMDAlgorithms/MDNorm.h
    inc/MantidMDAlgorithms/MDNormDirectSC.h
    inc/MantidMDAlgorithms/MDNormSCD.h
    inc/MantidMDAlgorithms/MDTransfAxisNames.h
    inc/MantidMDAlgorithms/MDTransfFactory.h
    inc/MantidMDAlgorithms/MDTransfInterface.h
    inc/MantidMDAlgorithms/MDTransfModQ.h
    inc/MantidMDAlgorithms/MDTransfNoQ.h
    inc/MantidMDAlgorithms/MDTransfQ3D.h
    inc/MantidMDAlgorithms/MDWSDescription.h
    inc/MantidMDAlgorithms/MDWSTransform.h
    inc/MantidMDAlgorithms/MagneticFormFactorCorrectionMD.h
    inc/MantidMDAlgorithms/DebyeWallerFactorCorrectionMD.h
    inc/MantidMDAlgorithms/MaskMD.h
    inc/MantidMDAlgorithms/MergeMD.h
    inc/MantidMDAlgorithms/MergeMDFiles.h
    inc/MantidMDAlgorithms/MinusMD.h
    inc/MantidMDAlgorithms/MultiplyMD.h
    inc/MantidMDAlgorithms/NotMD.h
    inc/MantidMDAlgorithms/OrMD.h
    inc/MantidMDAlgorithms/PlusMD.h
    inc/MantidMDAlgorithms/PolarizationAngleCorrectionMD.h
    inc/MantidMDAlgorithms/PowerMD.h
    inc/MantidMDAlgorithms/PreprocessDetectorsToMD.h
    inc/MantidMDAlgorithms/QTransform.h
    inc/MantidMDAlgorithms/QueryMDWorkspace.h
    inc/MantidMDAlgorithms/RecalculateTrajectoriesExtents.h
    inc/MantidMDAlgorithms/ReplicateMD.h
    inc/MantidMDAlgorithms/SaveIsawQvector.h
    inc/MantidMDAlgorithms/SaveMD.h
    inc/MantidMDAlgorithms/SaveMD2.h
    inc/MantidMDAlgorithms/SaveZODS.h
    inc/MantidMDAlgorithms/SetMDFrame.h
    inc/MantidMDAlgorithms/SetMDUsingMask.h
    inc/MantidMDAlgorithms/SliceMD.h
    inc/MantidMDAlgorithms/SlicingAlgorithm.h
    inc/MantidMDAlgorithms/SmoothMD.h
    inc/MantidMDAlgorithms/ThresholdMD.h
    inc/MantidMDAlgorithms/TransformMD.h
    inc/MantidMDAlgorithms/TransposeMD.h
    inc/MantidMDAlgorithms/UnaryOperationMD.h
    inc/MantidMDAlgorithms/UnitsConversionHelper.h
    inc/MantidMDAlgorithms/Vector3DParameter.h
    inc/MantidMDAlgorithms/Vector3DParameterParser.h
    inc/MantidMDAlgorithms/WeightedMeanMD.h
    inc/MantidMDAlgorithms/XorMD.h
)

# Test files. Other source files required.
set(TEST_FILES
    # these tests are as they test verify different parts of the CPR algorithms
    AccumulateMDTest.h
    AndMDTest.h
    ApplyDetailedBalanceMDTest.h
    BooleanBinaryOperationMDTest.h
    CalculateCoverageDGSTest.h
    CentroidPeaksMD2Test.h
    ChangeQConventionTest.h
    CloneMDWorkspaceTest.h
    CompactMDTest.h
    CompareMDWorkspacesTest.h
    ConvToMDEventsWSIndexingTest.h
    ConvertCWPDMDToSpectraTest.h
    ConvertCWSDExpToMomentumTest.h
    ConvertCWSDMDtoHKLTest.h
    ConvertEventsToMDTest.h
    ConvertHFIRSCDtoMDETest.h
    ConvertMDHistoToMatrixWorkspaceTest.h
    ConvertSpiceDataToRealSpaceTest.h
    ConvertToDetectorFaceMDTest.h
    ConvertToDiffractionMDWorkspace2Test.h
    ConvertToDiffractionMDWorkspace3Test.h
    ConvertToDiffractionMDWorkspaceTest.h
    ConvertToMDComponentsTest.h
    ConvertToMDMinMaxGlobalTest.h
    ConvertToMDMinMaxLocalTest.h
    ConvertToMDTest.h
    ConvertToQ3DdETest.h
    CreateMDHistoWorkspaceTest.h
    CreateMDTest.h
    CreateMDWorkspaceTest.h
    CutMDTest.h
    DgsScatteredTransmissionCorrectionMDTest.h
    DisplayNormalizationSetterTest.h
    DivideMDTest.h
    EqualToMDTest.h
    EvaluateMDFunctionTest.h
    ExponentialMDTest.h
    FakeMDEventDataTest.h
    FindPeaksMDTest.h
    FitMDTest.h
    FlippingRatioCorrectionMDTest.h
    GetSpiceDataRawCountsFromMDTest.h
    GreaterThanMDTest.h
    ImportMDEventWorkspaceTest.h
    ImportMDHistoWorkspaceTest.h
    Integrate3DEventsTest.h
    IntegrateEllipsoidsTest.h
    IntegrateEllipsoidsTwoStepTest.h
    IntegrateEllipsoidsWithSatellitesTest.h
    IntegrateFluxTest.h
    IntegrateMDHistoWorkspaceTest.h
    IntegratePeaksCWSDTest.h
    IntegratePeaksMD2Test.h
    IntegratePeaksMDHKLTest.h
    IntegratePeaksMDTest.h
    IntegrateQLabEventsTest.h
    InvalidParameterParserTest.h
    InvalidParameterTest.h
    LessThanMDTest.h
    LoadDNSSCDTest.h
    LoadGaussCubeTest.h
    LoadMDTest.h
    LoadSQW2Test.h
    LoadSQWTest.h
    LogarithmMDTest.h
    MDBoxMaskFunctionTest.h
    MDEventWSWrapperTest.h
    MDNormDirectSCTest.h
    MDNormSCDTest.h
    MDTransfAxisNamesTest.h
    MDTransfFactoryTest.h
    MDTransfModQTest.h
    MDTransfQ3DTest.h
    MDWSDescriptionTest.h
    MDWSTransfTest.h
    MagneticFormFactorCorrectionMDTest.h
    DebyeWallerFactorCorrectionMDTest.h
    MaskMDTest.h
    MergeMDFilesTest.h
    MergeMDTest.h
    MinusMDTest.h
    MultiplyMDTest.h
    NotMDTest.h
    OrMDTest.h
    PlusMDTest.h
    PolarizationAngleCorrectionMDTest.h
    PowerMDTest.h
    PreprocessDetectorsToMDTest.h
    QTransformTest.h
    QueryMDWorkspaceTest.h
    RecalculateTrajectoriesExtentsTest.h
    ReplicateMDTest.h
    SaveIsawQvectorTest.h
    SaveMD2Test.h
    SaveMDTest.h
    SaveZODSTest.h
    SetMDFrameTest.h
    SetMDUsingMaskTest.h
    SliceMDTest.h
    SlicingAlgorithmTest.h
    SmoothMDTest.h
    ThresholdMDTest.h
    TransformMDTest.h
    TransposeMDTest.h
    UnaryOperationMDTest.h
    UnitsConversionHelperTest.h
    WeightedMeanMDTest.h
    XorMDTest.h
)

set(GMOCK_TEST_FILES BinaryOperationMDTest.h BinMDTest.h)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Use a precompiled header where they are supported
enable_precompiled_headers(inc/MantidMDAlgorithms/PrecompiledHeader.h SRC_FILES)
# Add the target for this directory
add_library(MDAlgorithms ${SRC_FILES} ${INC_FILES})
add_library(Mantid::MDAlgorithms ALIAS MDAlgorithms)

target_include_directories(
  MDAlgorithms PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(MDAlgorithms PROPERTIES OUTPUT_NAME MantidMDAlgorithms COMPILE_DEFINITIONS IN_MANTID_MDALGORITHMS)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(MDAlgorithms PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(MDAlgorithms PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET MDAlgorithms PROPERTY FOLDER "MantidFramework")

target_link_libraries(MDAlgorithms PUBLIC Mantid::API Mantid::DataObjects Mantid::Kernel Mantid::Geometry)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  MDAlgorithms PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/MDAlgorithms> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(MDAlgorithms TRUE)

# Installation settings
mtd_install_framework_lib(TARGETS MDAlgorithms PLUGIN_LIB)
