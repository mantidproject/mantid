set(SRC_FILES src/BroadcastWorkspace.cpp src/GatherWorkspaces.cpp src/Chunking.cpp)

set(INC_FILES inc/MantidMPIAlgorithms/BroadcastWorkspace.h inc/MantidMPIAlgorithms/GatherWorkspaces.h
              inc/MantidMPIAlgorithms/MPISerialization.h inc/MantidMPIAlgorithms/Chunking.h
)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(MPIAlgorithms ${SRC_FILES} ${INC_FILES})
add_library(Mantid::MPIAlgorithms ALIAS MPIAlgorithms)

target_include_directories(
  MPIAlgorithms PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(MPIAlgorithms PROPERTIES OUTPUT_NAME MantidMPIAlgorithms)
# Add to the 'Framework' group in VS
set_property(TARGET Algorithms PROPERTY FOLDER "MantidFramework")

target_link_libraries(
  MPIAlgorithms
  PUBLIC Mantid::API Mantid::DataObjects
  PRIVATE Mantid::Kernel
)
if(MPI_BUILD)
  message(STATUS "Boost_MPI_LIBRARY = ${Boost_MPI_LIBRARY}")
  target_compile_definitions(MPIAlgorithms PRIVATE MPI_BUILD)
  target_link_libraries(
    MPIAlgorithms
    PUBLIC ${Boost_MPI_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    PRIVATE ${MPI_CXX_LIBRARIES}
  )
  # Add the unit tests directory
  add_subdirectory(test)
endif()

target_include_directories(
  MPIAlgorithms
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
         $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/MPIAlgorithms> $<INSTALL_INTERFACE:include/Mantid>
)

generate_mantid_export_header(MPIAlgorithms TRUE)

# Installation settings
mtd_install_targets(TARGETS MPIAlgorithms INSTALL_DIRS ${PLUGINS_DIR} ${WORKBENCH_PLUGINS_DIR})
