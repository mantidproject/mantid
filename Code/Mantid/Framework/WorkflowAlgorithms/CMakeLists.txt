set ( SRC_FILES
        src/EQSANSQ2D.cpp
)
                
set ( SRC_UNITY_IGNORE_FILES 
)

set ( INC_FILES
        inc/MantidWorkflowAlgorithms/EQSANSQ2D.h
)
              
set ( TEST_FILES
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
endif ()

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( PythonEQSANSTest.py ${TEST_PY_FILES} )
  add_dependencies ( PythonEQSANSTest.py PythonAPI )
  add_dependencies ( FrameworkTests PythonEQSANSTest.py )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonEQSANSTest.py PROPERTY FOLDER "UnitTests" )
endif ()


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS WorkflowAlgorithms DESTINATION ${PLUGINS_DIR} )
