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
add_library(MPIAlgorithms SHARED ${SRC_FILES} ${INC_FILES})
add_library(Mantid::MPIAlgorithms ALIAS MPIAlgorithms)

target_include_directories(
  MPIAlgorithms PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(MPIAlgorithms PROPERTIES OUTPUT_NAME MantidMPIAlgorithms)
# Add to the 'Framework' group in VS
set_property(TARGET MPIAlgorithms PROPERTY FOLDER "MantidFramework")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(MPIAlgorithms PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
else()
  message(STATUS "Only linux builds are currently supported")
endif()

target_link_libraries(
  MPIAlgorithms
  PUBLIC Mantid::API Mantid::DataObjects
  PRIVATE Mantid::Kernel
)
if(MPI_BUILD)
  target_sources(MPIAlgorithms PRIVATE src/MPILifetime.cpp)
  message(STATUS "Boost_MPI_LIBRARY = ${Boost_MPI_LIBRARY}")
  target_compile_definitions(MPIAlgorithms PUBLIC MPI_BUILD)
  target_link_libraries(
    MPIAlgorithms
    PUBLIC ${Boost_MPI_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    PRIVATE ${MPI_CXX_LIBRARIES}
  )
  # Add the unit tests directory
  add_subdirectory(test)
  # libMantidMPIAlgorithms.so is always built so documentation for the algorithms can be built
  message(STATUS "Installing MPIAlgorithms")
  mtd_install_framework_lib(TARGETS MPIAlgorithms PLUGIN_LIB)
endif()

target_include_directories(
  MPIAlgorithms
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
         $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/MPIAlgorithms> $<INSTALL_INTERFACE:include/Mantid>
)

generate_mantid_export_header(MPIAlgorithms TRUE)
