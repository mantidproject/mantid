set ( SRC_FILES
        src/EQSANSQ2D.cpp
        src/SANSSolidAngleCorrection.cpp
)
                
set ( SRC_UNITY_IGNORE_FILES 
)

set ( INC_FILES
        inc/MantidWorkflowAlgorithms/EQSANSQ2D.h
        inc/MantidWorkflowAlgorithms/SANSSolidAngleCorrection.h
)
              
set ( TEST_FILES
		test/SANSSolidAngleCorrectionTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(WorkflowAlgorithms SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

set ( TEST_PY_FILES test/EQSANSQ2DTest.py )

# Add the target for this directory
add_library ( WorkflowAlgorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( WorkflowAlgorithms PROPERTIES OUTPUT_NAME MantidWorkflowAlgorithms )
# Add to the 'Framework' group in VS
set_property ( TARGET WorkflowAlgorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( WorkflowAlgorithms ${MANTIDLIBS} ${GSL_LIBRARIES} )

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
