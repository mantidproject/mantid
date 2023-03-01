message(STATUS "This is an MPI-enabled build of the Mantid Framework!")

# This finds OpenMPI on rhel6 (assuming it's installed, of course)
find_package(MPI REQUIRED)
# The FindMPI module doesn't seem to respect the REQUIRED specifier (as of CMake 2.8.7)
if(NOT MPI_FOUND)
  message(SEND_ERROR "MPI_BUILD flag is ON, but MPI could not be found")
endif()

# Set things up to use the OpenMPI 'wrapper' compilers. With CMake, we don't use the OpenMPI 'wrapper' compilers
# (because setting CMAKE_CXX_COMPILER after the 'project' call is a no-no), but rather extract the extra arguments and
# apply them to the normal compiler.
include_directories(SYSTEM ${MPI_CXX_INCLUDE_PATH})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" ${MPI_CXX_COMPILE_FLAGS})
# Setting the linker flags doesn't seem to work right (or matter) set ( CMAKE_SHARED_LINKER_FLAGS
# "${CMAKE_SHARED_LINKER_FLAGS}" ${MPI_CXX_LINK_FLAGS} )

set(BOOST_ROOT /usr/lib64/openmpi) # This is where (boost-)openmpi winds up on rhel6
find_package(
  Boost
  COMPONENTS mpi serialization
  REQUIRED
)
include_directories(${Boost_INCLUDE_DIRS})
# unset ( BOOST_ROOT )

# Add a definition that's used to guard MPI-specific parts of the main code
add_definitions(-DMPI_BUILD)

# Configure a module file This is where the software tree will be that we want the modulefile to point into. The
# 'normal' install location is the default, but this should normally be set manually in the cmake configuration for a
# build
set(INSTALL_ROOT
    ${CMAKE_INSTALL_PREFIX}
    CACHE PATH "The location you intend to install into"
)
# Remove old module file
execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/modulefile)
