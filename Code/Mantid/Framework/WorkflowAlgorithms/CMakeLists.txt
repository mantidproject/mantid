set ( SRC_FILES
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
	src/EQSANSInstrument.cpp
	src/EQSANSLoad.cpp
	src/EQSANSMonitorTOF.cpp
	src/EQSANSPatchSensitivity.cpp
	src/EQSANSQ2D.cpp
	src/HFIRDarkCurrentSubtraction.cpp
	src/HFIRInstrument.cpp
	src/HFIRLoad.cpp
	src/HFIRSANSNormalise.cpp
	src/MuonCalculateAsymmetry.cpp
	src/MuonLoad.cpp
	src/RefReduction.cpp
	src/RefRoi.cpp
	src/SANSBeamFinder.cpp
	src/SANSBeamFluxCorrection.cpp
	src/SANSSensitivityCorrection.cpp
	src/SANSSolidAngleCorrection.cpp
	src/SendUsage.cpp
	src/SetupEQSANSReduction.cpp
	src/SetupHFIRReduction.cpp
	src/SetupILLD33Reduction.cpp
	src/StepScan.cpp
	src/WorkflowAlgorithmHelpers.cpp
)
                
set ( SRC_UNITY_IGNORE_FILES 
)

set ( INC_FILES
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
	inc/MantidWorkflowAlgorithms/EQSANSInstrument.h
	inc/MantidWorkflowAlgorithms/EQSANSLoad.h
	inc/MantidWorkflowAlgorithms/EQSANSMonitorTOF.h
	inc/MantidWorkflowAlgorithms/EQSANSPatchSensitivity.h
	inc/MantidWorkflowAlgorithms/EQSANSQ2D.h
	inc/MantidWorkflowAlgorithms/EQSANSReduce.h
	inc/MantidWorkflowAlgorithms/HFIRDarkCurrentSubtraction.h
	inc/MantidWorkflowAlgorithms/HFIRInstrument.h
	inc/MantidWorkflowAlgorithms/HFIRLoad.h
	inc/MantidWorkflowAlgorithms/HFIRSANSNormalise.h
	inc/MantidWorkflowAlgorithms/MuonCalculateAsymmetry.h
	inc/MantidWorkflowAlgorithms/MuonLoad.h
	inc/MantidWorkflowAlgorithms/RefReduction.h
	inc/MantidWorkflowAlgorithms/RefRoi.h
	inc/MantidWorkflowAlgorithms/SANSBeamFinder.h
	inc/MantidWorkflowAlgorithms/SANSBeamFluxCorrection.h
	inc/MantidWorkflowAlgorithms/SANSSensitivityCorrection.h
	inc/MantidWorkflowAlgorithms/SANSSolidAngleCorrection.h
	inc/MantidWorkflowAlgorithms/SendUsage.h
	inc/MantidWorkflowAlgorithms/SetupEQSANSReduction.h
	inc/MantidWorkflowAlgorithms/SetupHFIRReduction.h
	inc/MantidWorkflowAlgorithms/SetupILLD33Reduction.h
	inc/MantidWorkflowAlgorithms/StepScan.h
	inc/MantidWorkflowAlgorithms/WorkflowAlgorithmHelpers.h
)
              
set ( TEST_FILES
	MuonCalculateAsymmetryTest.h
	MuonLoadTest.h
	SANSSolidAngleCorrectionTest.h
	SendUsageTest.h
	StepScanTest.h
)

set ( TEST_PY_FILES 
  EQSANSQ2DTest.py 
  SANSBeamFluxCorrectionTest.py 
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(WorkflowAlgorithms SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidWorkflowAlgorithms/PrecompiledHeader.h SRC_FILES )

# Add the target for this directory
add_library ( WorkflowAlgorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( WorkflowAlgorithms PROPERTIES OUTPUT_NAME MantidWorkflowAlgorithms 
                                                      INSTALL_RPATH "@loader_path/../Contents/MacOS/")
# Add to the 'Framework' group in VS
set_property ( TARGET WorkflowAlgorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc ../Nexus/inc )

target_link_libraries ( WorkflowAlgorithms ${MANTIDLIBS} Nexus ${GSL_LIBRARIES} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS WorkflowAlgorithms ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
