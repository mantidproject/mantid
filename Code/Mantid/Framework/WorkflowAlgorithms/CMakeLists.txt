set ( SRC_FILES
	src/ComputeSensitivity.cpp
	src/DgsConvertToEnergyTransfer.cpp
	src/DgsReduction.cpp
	src/EQSANSDarkCurrentSubtraction.cpp
	src/EQSANSInstrument.cpp
	src/EQSANSLoad.cpp
	src/EQSANSPatchSensitivity.cpp
	src/EQSANSQ2D.cpp
	src/EQSANSReduce.cpp
	src/HFIRDarkCurrentSubtraction.cpp
	src/HFIRInstrument.cpp
	src/HFIRLoad.cpp
	src/RefReduction.cpp
	src/RefRoi.cpp
	src/SANSBeamFinder.cpp
	src/SANSSensitivityCorrection.cpp
	src/SANSSolidAngleCorrection.cpp
	src/SetupEQSANSReduction.cpp
	src/SetupHFIRReduction.cpp
)
                
set ( SRC_UNITY_IGNORE_FILES 
)

set ( INC_FILES
	inc/MantidWorkflowAlgorithms/ComputeSensitivity.h
	inc/MantidWorkflowAlgorithms/DgsConvertToEnergyTransfer.h
	inc/MantidWorkflowAlgorithms/DgsReduction.h
	inc/MantidWorkflowAlgorithms/EQSANSDarkCurrentSubtraction.h
	inc/MantidWorkflowAlgorithms/EQSANSInstrument.h
	inc/MantidWorkflowAlgorithms/EQSANSLoad.h
	inc/MantidWorkflowAlgorithms/EQSANSPatchSensitivity.h
	inc/MantidWorkflowAlgorithms/EQSANSQ2D.h
	inc/MantidWorkflowAlgorithms/EQSANSReduce.h
	inc/MantidWorkflowAlgorithms/HFIRDarkCurrentSubtraction.h
	inc/MantidWorkflowAlgorithms/HFIRInstrument.h
	inc/MantidWorkflowAlgorithms/HFIRLoad.h
	inc/MantidWorkflowAlgorithms/RefReduction.h
	inc/MantidWorkflowAlgorithms/RefRoi.h
	inc/MantidWorkflowAlgorithms/SANSBeamFinder.h
	inc/MantidWorkflowAlgorithms/SANSSensitivityCorrection.h
	inc/MantidWorkflowAlgorithms/SANSSolidAngleCorrection.h
	inc/MantidWorkflowAlgorithms/SetupEQSANSReduction.h
	inc/MantidWorkflowAlgorithms/SetupHFIRReduction.h
)
              
set ( TEST_FILES
		test/SANSSolidAngleCorrectionTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(WorkflowAlgorithms SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

set ( TEST_PY_FILES test/EQSANSQ2DTest.py )

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidWorkflowAlgorithms/PrecompiledHeader.h MantidWorkflowAlgorithms src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
# Add the target for this directory
add_library ( WorkflowAlgorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( WorkflowAlgorithms PROPERTIES OUTPUT_NAME MantidWorkflowAlgorithms )
# Add to the 'Framework' group in VS
set_property ( TARGET WorkflowAlgorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc ../Nexus/inc )

target_link_libraries ( WorkflowAlgorithms ${MANTIDLIBS} Nexus ${GSL_LIBRARIES} )

if ( CXXTEST_FOUND )
  # Make the rest of the tests
  include_directories ( ../DataHandling/inc )
  cxxtest_add_test ( WorkflowAlgorithmsTest ${TEST_FILES} )
  target_link_libraries ( WorkflowAlgorithmsTest WorkflowAlgorithms Algorithms DataHandling )
  add_dependencies ( FrameworkTests WorkflowAlgorithmsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET WorkflowAlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( PythonWorkflowAlgorithmsTest.py ${TEST_PY_FILES} )
  add_dependencies ( PythonWorkflowAlgorithmsTest.py PythonAPI )
  add_dependencies ( FrameworkTests PythonWorkflowAlgorithmsTest.py )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonWorkflowAlgorithmsTest.py PROPERTY FOLDER "UnitTests" )
endif ()


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS WorkflowAlgorithms ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
