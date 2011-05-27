set ( SRC_FILES src/BroadcastWorkspace.cpp
                src/GatherWorkspaces.cpp
)

set ( INC_FILES inc/MantidMPIAlgorithms/BroadcastWorkspace.h
                inc/MantidMPIAlgorithms/GatherWorkspaces.h
)

set ( TEST_FILES test/BroadcastWorkspaceTest.h
                 test/GatherWorkspacesTest.h
)

# Add the target for this directory
add_library ( MPIAlgorithms ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( MPIAlgorithms PROPERTIES OUTPUT_NAME MantidMPIAlgorithms )
# Add to the 'Framework' group in VS
set_property ( TARGET Algorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( MPIAlgorithms ${MANTIDLIBS} )

if ( CXXTEST_FOUND )
  include_directories ( ../TestHelpers/inc )
  cxxtest_add_test ( MPIAlgorithmsTest ${TEST_FILES} )
  target_link_libraries ( MPIAlgorithmsTest MPIAlgorithms TestHelpers )
  add_dependencies ( FrameworkTests MPIAlgorithmsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MPIAlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

# install ( TARGETS MPIAlgorithms DESTINATION ${PLUGINS_DIR} )
