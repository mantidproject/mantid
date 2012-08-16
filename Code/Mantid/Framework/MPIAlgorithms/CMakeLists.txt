set ( SRC_FILES src/BroadcastWorkspace.cpp
                src/GatherWorkspaces.cpp
)

set ( INC_FILES inc/MantidMPIAlgorithms/BroadcastWorkspace.h
                inc/MantidMPIAlgorithms/GatherWorkspaces.h
                inc/MantidMPIAlgorithms/MPISerialization.h
)

set ( TEST_FILES BroadcastWorkspaceTest.h
                 GatherWorkspacesTest.h
)

# Add the target for this directory
add_library ( MPIAlgorithms ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( MPIAlgorithms PROPERTIES OUTPUT_NAME MantidMPIAlgorithms )
# Add to the 'Framework' group in VS
set_property ( TARGET Algorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( MPIAlgorithms ${MANTIDLIBS} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MPIAlgorithms ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
