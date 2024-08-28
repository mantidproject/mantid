set(SRC_FILES
    src/AlignAndFocusPowder.cpp
    src/ComputeSensitivity.cpp
    src/DgsAbsoluteUnitsReduction.cpp
    src/DgsConvertToEnergyTransfer.cpp
    src/DgsDiagnose.cpp
    src/DgsPreprocessData.cpp
    src/DgsProcessDetectorVanadium.cpp
    src/DgsReduction.cpp
    src/DgsRemap.cpp
    src/EQSANSDarkCurrentSubtraction.cpp
    src/EQSANSDarkCurrentSubtraction2.cpp
    src/EQSANSInstrument.cpp
    src/EQSANSLoad.cpp
    src/EQSANSMonitorTOF.cpp
    src/EQSANSPatchSensitivity.cpp
    src/EQSANSQ2D.cpp
    src/ExtractQENSMembers.cpp
    src/HFIRDarkCurrentSubtraction.cpp
    src/HFIRInstrument.cpp
    src/HFIRLoad.cpp
    src/HFIRSANSNormalise.cpp
    src/IMuonAsymmetryCalculator.cpp
    src/LoadEventAndCompress.cpp
    src/MuonGroupAsymmetryCalculator.cpp
    src/MuonGroupCalculator.cpp
    src/MuonGroupCountsCalculator.cpp
    src/MuonProcess.cpp
    src/MuonPairAsymmetryCalculator.cpp
    src/ProcessIndirectFitParameters.cpp
    src/SANSBeamFinder.cpp
    src/SANSBeamFluxCorrection.cpp
    src/SANSSensitivityCorrection.cpp
    src/SANSSolidAngleCorrection.cpp
    src/SetupEQSANSReduction.cpp
    src/SetupHFIRReduction.cpp
    src/SofTwoThetaTOF.cpp
    src/StepScan.cpp
    src/WorkflowAlgorithmHelpers.cpp
)

set(INC_FILES
    inc/MantidWorkflowAlgorithms/AlignAndFocusPowder.h
    inc/MantidWorkflowAlgorithms/ComputeSensitivity.h
    inc/MantidWorkflowAlgorithms/DgsAbsoluteUnitsReduction.h
    inc/MantidWorkflowAlgorithms/DgsConvertToEnergyTransfer.h
    inc/MantidWorkflowAlgorithms/DgsDiagnose.h
    inc/MantidWorkflowAlgorithms/DgsPreprocessData.h
    inc/MantidWorkflowAlgorithms/DgsProcessDetectorVanadium.h
    inc/MantidWorkflowAlgorithms/DgsReduction.h
    inc/MantidWorkflowAlgorithms/DgsRemap.h
    inc/MantidWorkflowAlgorithms/EQSANSDarkCurrentSubtraction.h
    inc/MantidWorkflowAlgorithms/EQSANSDarkCurrentSubtraction2.h
    inc/MantidWorkflowAlgorithms/EQSANSInstrument.h
    inc/MantidWorkflowAlgorithms/EQSANSLoad.h
    inc/MantidWorkflowAlgorithms/EQSANSMonitorTOF.h
    inc/MantidWorkflowAlgorithms/EQSANSPatchSensitivity.h
    inc/MantidWorkflowAlgorithms/EQSANSQ2D.h
    inc/MantidWorkflowAlgorithms/ExtractQENSMembers.h
    inc/MantidWorkflowAlgorithms/HFIRDarkCurrentSubtraction.h
    inc/MantidWorkflowAlgorithms/HFIRInstrument.h
    inc/MantidWorkflowAlgorithms/HFIRLoad.h
    inc/MantidWorkflowAlgorithms/HFIRSANSNormalise.h
    inc/MantidWorkflowAlgorithms/IMuonAsymmetryCalculator.h
    inc/MantidWorkflowAlgorithms/LoadEventAndCompress.h
    inc/MantidWorkflowAlgorithms/MuonGroupAsymmetryCalculator.h
    inc/MantidWorkflowAlgorithms/MuonGroupCalculator.h
    inc/MantidWorkflowAlgorithms/MuonGroupCountsCalculator.h
    inc/MantidWorkflowAlgorithms/MuonProcess.h
    inc/MantidWorkflowAlgorithms/MuonPairAsymmetryCalculator.h
    inc/MantidWorkflowAlgorithms/ProcessIndirectFitParameters.h
    inc/MantidWorkflowAlgorithms/SANSBeamFinder.h
    inc/MantidWorkflowAlgorithms/SANSBeamFluxCorrection.h
    inc/MantidWorkflowAlgorithms/SANSSensitivityCorrection.h
    inc/MantidWorkflowAlgorithms/SANSSolidAngleCorrection.h
    inc/MantidWorkflowAlgorithms/SetupEQSANSReduction.h
    inc/MantidWorkflowAlgorithms/SetupHFIRReduction.h
    inc/MantidWorkflowAlgorithms/SofTwoThetaTOF.h
    inc/MantidWorkflowAlgorithms/StepScan.h
    inc/MantidWorkflowAlgorithms/WorkflowAlgorithmHelpers.h
)

set(TEST_FILES
    AlignAndFocusPowderTest.h
    ExtractQENSMembersTest.h
    IMuonAsymmetryCalculatorTest.h
    LoadEventAndCompressTest.h
    MuonProcessTest.h
    ProcessIndirectFitParametersTest.h
    SANSSolidAngleCorrectionTest.h
    SofTwoThetaTOFTest.h
    StepScanTest.h
)

set(TEST_PY_FILES EQSANSQ2DTest.py SANSBeamFluxCorrectionTest.py)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Use a precompiled header where they are supported
enable_precompiled_headers(inc/MantidWorkflowAlgorithms/PrecompiledHeader.h SRC_FILES)

# Add the target for this directory
add_library(WorkflowAlgorithms ${SRC_FILES} ${INC_FILES})
add_library(Mantid::WorkflowAlgorithms ALIAS WorkflowAlgorithms)

target_include_directories(
  WorkflowAlgorithms PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)
# Set the name of the generated library
set_target_properties(WorkflowAlgorithms PROPERTIES OUTPUT_NAME MantidWorkflowAlgorithms)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(WorkflowAlgorithms PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(WorkflowAlgorithms PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET WorkflowAlgorithms PROPERTY FOLDER "MantidFramework")

target_link_libraries(
  WorkflowAlgorithms
  PUBLIC Mantid::Nexus Mantid::API Mantid::DataObjects
  PRIVATE Mantid::Kernel
)

# Add the unit tests directory
add_subdirectory(test)

# Installation settings
mtd_install_framework_lib(TARGETS WorkflowAlgorithms PLUGIN_LIB)
