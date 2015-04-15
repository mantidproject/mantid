message ( STATUS "This is an MPI-enabled build of the Mantid Framework!" )

# This finds OpenMPI on rhel6 (assuming it's installed, of course)
find_package ( MPI REQUIRED )
# The FindMPI module doesn't seem to respect the REQUIRED specifier (as of CMake 2.8.7)
if ( NOT MPI_FOUND )
  message ( SEND_ERROR "MPI_BUILD flag is ON, but MPI could not be found" )
endif ()

# Set things up to use the OpenMPI 'wrapper' compilers.
# With CMake, we don't use the OpenMPI 'wrapper' compilers (because setting CMAKE_CXX_COMPILER after the
# 'project' call is a no-no), but rather extract the extra arguments and apply them to the normal compiler.
include_directories ( SYSTEM ${MPI_CXX_INCLUDE_PATH} )
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" ${MPI_CXX_COMPILE_FLAGS} )
# Setting the linker flags doesn't seem to work right (or matter)
#set ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}" ${MPI_CXX_LINK_FLAGS} ) 
  
set ( BOOST_ROOT /usr/lib64/openmpi ) # This is where (boost-)openmpi winds up on rhel6
find_package ( Boost COMPONENTS mpi mpi_python serialization REQUIRED )
include_directories( ${Boost_INCLUDE_DIRS} )

#  FIND_LIBRARY(BOOST_MPI_LIBRARY
#    NAMES boost_mpi-mt
#    PATHS "/usr/lib64/openmpi/lib"
#    PATH_SUFFIXES lib
#    DOC "Boost MPI library" )  
    
#message ( STATUS "BOOST MPI LIBRARY: ${BOOST_MPI_LIBRARY}" )

#set ( MANTIDLIBS  ${MANTIDLIBS} ${BOOST_MPI_LIBRARY} )
    
# Add a definition that's used to guard MPI-specific parts of the main code
add_definitions ( -DMPI_BUILD )
    
# Add the ability to build a 'mantid-mpi' rpm
set ( CPACK_PACKAGE_NAME mantid-mpi )
include ( CPackCommon )
include ( CPackLinuxSetup )
# Framework dependencies
set ( CPACK_RPM_PACKAGE_REQUIRES "boost >= 1.34.1,nexus,gsl,glibc,OpenCASCADE-libs-modelling >= 6.3.0,OpenCASCADE-libs-foundation >= 6.3.0,muParser,numpy" )
set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},poco-crypto,poco-util,poco-xml,poco-net,poco-netssl,poco-foundation" )
set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},gperftools-libs >= 2.0" )
set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},boost-openmpi" )

# Configure a module file
# This is where the software tree will be that we want the modulefile to point into. The 'normal' install location is the default,
# but this should normally be set manually in the cmake configuration for a build
set ( INSTALL_ROOT ${CMAKE_INSTALL_PREFIX} CACHE PATH "The location you intend to install into")
# Remove old module file
execute_process ( COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/modulefile )
# Create new module file
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/../Installers/LinuxInstaller/modulefile.template
                ${CMAKE_BINARY_DIR}/modulefile/${CPACK_PACKAGE_VERSION}
)

include ( CPack )

