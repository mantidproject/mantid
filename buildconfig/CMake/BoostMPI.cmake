message ( STATUS "This is an MPI-enabled build of the Mantid Framework!" )

find_package ( MPI REQUIRED )
# The FindMPI module doesn't seem to respect the REQUIRED specifier (as of CMake 2.8.7)
if ( NOT MPI_CXX_FOUND )
  message ( SEND_ERROR "C++ MPI libraries could not be found" )
endif ()

include_directories ( SYSTEM ${MPI_CXX_INCLUDE_PATH} )
set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS} )

find_package ( Boost REQUIRED mpi serialization )
include_directories( ${Boost_INCLUDE_DIRS} )

set ( MANTIDLIBS ${MANTIDLIBS} ${Boost_LIBRARIES} ${MPI_CXX_LIBRARIES} )

add_definitions ( -DMPI_EXPERIMENTAL )
