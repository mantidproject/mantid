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
  enable_unity_build(WorkflowAlgorithm SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( WorkflowAlgorithm ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( WorkflowAlgorithm PROPERTIES OUTPUT_NAME MantidWorkflowAlgorithm )
# Add to the 'Framework' group in VS
set_property ( TARGET WorkflowAlgorithm PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( WorkflowAlgorithm ${MANTIDLIBS} ${GSL_LIBRARIES} )

if ( CXXTEST_FOUND )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS WorkflowAlgorithm DESTINATION ${PLUGINS_DIR} )
