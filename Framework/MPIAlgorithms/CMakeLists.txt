set(SRC_FILES src/BroadcastWorkspace.cpp src/GatherWorkspaces.cpp)

set(INC_FILES
    inc/MantidMPIAlgorithms/BroadcastWorkspace.h
    inc/MantidMPIAlgorithms/GatherWorkspaces.h
    inc/MantidMPIAlgorithms/MPISerialization.h)

set(TEST_FILES BroadcastWorkspaceTest.h GatherWorkspacesTest.h)

if(COVERALLS)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(MPIAlgorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties(MPIAlgorithms PROPERTIES OUTPUT_NAME MantidMPIAlgorithms)
# Add to the 'Framework' group in VS
set_property(TARGET Algorithms PROPERTY FOLDER "MantidFramework")

include_directories(inc)

target_link_libraries(MPIAlgorithms ${MANTIDLIBS})

# Add the unit tests directory
add_subdirectory(test)

#
# Installation settings
#

mtd_install_targets(TARGETS
                    MPIAlgorithms
                    INSTALL_DIRS
                    ${PLUGINS_DIR}
                    ${WORKBENCH_PLUGINS_DIR})
