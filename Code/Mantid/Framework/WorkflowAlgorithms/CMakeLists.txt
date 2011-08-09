set ( SRC_FILES
        src/EQSANSQ2D.cpp
        src/EQSANSResolution.cpp
        src/SANSSolidAngleCorrection.cpp
        src/EQSANSSensitivityCorrection.cpp
)
                
set ( SRC_UNITY_IGNORE_FILES 
)

set ( INC_FILES
        inc/MantidWorkflowAlgorithms/EQSANSQ2D.h
        inc/MantidWorkflowAlgorithms/EQSANSResolution.h
        inc/MantidWorkflowAlgorithms/SANSSolidAngleCorrection.h
        inc/MantidWorkflowAlgorithms/EQSANSSensitivityCorrection.h
)
              
set ( TEST_FILES
		test/SANSSolidAngleCorrectionTest.h
)

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidWorkflowAlgorithms/PrecompiledHeader.h MantidWorkflowAlgorithms WorkflowAlgorithms SRC_FILES INC_FILES )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(WorkflowAlgorithms SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

set ( TEST_PY_FILES test/EQSANSQ2DTest.py test/EQSANSSensitivityCorrectionTest.py )

# Add the target for this directory
add_library ( WorkflowAlgorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( WorkflowAlgorithms PROPERTIES OUTPUT_NAME MantidWorkflowAlgorithms )
# Add to the 'Framework' group in VS
set_property ( TARGET WorkflowAlgorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc ../Algorithms/inc)

target_link_libraries ( WorkflowAlgorithms Algorithms ${MANTIDLIBS} ${GSL_LIBRARIES} )

if ( CXXTEST_FOUND )
  # Make the rest of the tests
  include_directories ( ../DataHandling/inc ../TestHelpers/inc )
  cxxtest_add_test ( WorkflowAlgorithmsTest ${TEST_FILES} )
  target_link_libraries ( WorkflowAlgorithmsTest WorkflowAlgorithms Algorithms DataHandling Nexus TestHelpers )
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

install ( TARGETS WorkflowAlgorithms DESTINATION ${PLUGINS_DIR} )
