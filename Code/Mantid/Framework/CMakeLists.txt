# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required (VERSION 2.6)

# Add the path to our custom 'find' modules
set ( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Build/CMake")

# Define the project name. Allows building framework as a separate project.
project ( MantidFramework )

# Set paths to Third_Party for Windows and Mac builds
if ( NOT THIRD_PARTY )
  set ( THIRD_PARTY "${PROJECT_SOURCE_DIR}/../../Third_Party" )
  if ( WIN32 )
    include ( WindowsSetup )
  elseif ( APPLE )
    include ( DarwinSetup )
  endif ()
endif ()

# If building as a stand-alone project, call our common setup script
if ( NOT COMMONSETUP_DONE )
  include ( CommonSetup )
endif ()

###########################################################################
# Look for dependencies - bail out if any necessary ones not found
###########################################################################

# gsl is currently needed by Geometry, Algorithms & Curvefitting
find_package ( GSL REQUIRED )

###########################################################################
# Add OpenMP flags here - don't want them on QtPropertyBrowser
###########################################################################
if ( OPENMP_FOUND )
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}" )
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}" )
endif ()

###########################################################################
# **EXPERIMENTAL** MPI setup
###########################################################################

# Only available on Linux (specifically RedHat), and for a framework-only build
if ( ${CMAKE_PROJECT_NAME} MATCHES "MantidFramework" AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
  set ( MPI_BUILD OFF CACHE BOOL "Enable MPI options" )
  if ( MPI_BUILD )
    # This finds OpenMPI on rhel6 (assuming it's installed, of course)
    find_package ( MPI REQUIRED )
    # The FindMPI module doesn't seem to respect the REQUIRED specifier (as of CMake 2.8.7)
    if ( NOT MPI_FOUND )
      message ( SEND_ERROR "MPI_BUILD flag is ON, but MPI could not be found" )
    endif ()
    # Set things up to use the OpenMPI 'wrapper' compilers
    if ( MPI_CXX_COMPILER )
      set ( CMAKE_C_COMPILER ${MPI_C_COMPILER} )
      set ( CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER} )
    else ()
      # Handle older FindMPI.cmake
      set ( CMAKE_C_COMPILER ${MPI_COMPILER} )
      set ( CMAKE_CXX_COMPILER ${MPI_COMPILER} )
    endif ()
    include_directories ( SYSTEM ${MPI_INCLUDE_PATH} )
  
    set ( BOOST_ROOT /usr/lib64/openmpi ) # This is where (boost-)openmpi winds up on rhel6
    find_package ( Boost REQUIRED mpi serialization )
    include_directories( ${Boost_INCLUDE_DIRS} )
    
    # Add a definition that's used to guard MPI-specific parts of the main code
    add_definitions ( -DMPI_BUILD )
    
    # Add the ability to build a 'mantid-mpi' rpm
    set ( CPACK_PACKAGE_NAME mantid-mpi )
    include ( CPackCommon )
    include ( CPackLinuxSetup )
    # Framework dependencies
    set ( CPACK_RPM_PACKAGE_REQUIRES "boost >= 1.34.1,nexus,gsl,glibc,OpenCASCADE-libs-modelling >= 6.3.0,OpenCASCADE-libs-foundation >= 6.3.0,OpenCASCADE-libs-visualization >= 6.3.0,OpenCASCADE-libs-ocaf >= 6.3.0,OpenCASCADE-libs-ocaf-lite >= 6.3.0,muParser,numpy" )
    set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},poco-crypto,poco-data,poco-mysql,poco-sqlite,poco-odbc,poco-util,poco-xml,poco-zip,poco-net,poco-netssl,poco-foundation" )
    set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},gperftools-libs >= 2.0" )
    set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},boost-openmpi,boost-openmpi-python" )
    include ( CPack )
  endif ( MPI_BUILD )
endif ()

###########################################################################
# Globally-linked libraries variable
###########################################################################

# Might just as well link everything to Boost & Poco (found in CommonSetup)
# TCMalloc variable will be empty if not using
set ( MANTIDLIBS  ${Boost_LIBRARIES} ${POCO_LIBRARIES} ${TCMALLOC_LIBRARY} )

###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################

add_custom_target ( FrameworkTests ) # target for all framework tests
add_dependencies ( check FrameworkTests )

include_directories (NexusCPP/inc)
add_subdirectory (NexusCPP)
set ( MANTIDLIBS ${MANTIDLIBS} NexusCPP )

include_directories (Kernel/inc)
add_subdirectory (Kernel)
set ( MANTIDLIBS ${MANTIDLIBS} Kernel )

include_directories (Geometry/inc)
# muParser needed by Geometry and subsequent packages
include_directories ( ${MUPARSER_INCLUDE_DIR} )
set ( MANTIDLIBS ${MANTIDLIBS} ${MUPARSER_LIBRARIES} )
add_subdirectory (Geometry)
set ( MANTIDLIBS ${MANTIDLIBS} Geometry )

include_directories (API/inc)
add_subdirectory (API)
set ( MANTIDLIBS ${MANTIDLIBS} API )

add_subdirectory (PythonAPI)
add_subdirectory (PythonInterface)

find_package ( Matlab )
if( MATLAB_FOUND )
  add_subdirectory (MatlabAPI)
endif ()

include_directories (DataObjects/inc)
add_subdirectory (DataObjects)
set ( MANTIDLIBS ${MANTIDLIBS} DataObjects )

add_subdirectory (Nexus)
add_subdirectory (DataHandling)
add_subdirectory (Algorithms)
add_subdirectory (WorkflowAlgorithms)
add_subdirectory (CurveFitting)
add_subdirectory (Crystal)
add_subdirectory (ICat)

# If an MPI-enabled build, add in the MPI-specific algorithms package
if ( MPI_BUILD )
  add_subdirectory ( MPIAlgorithms )
endif ()

# If a OpenCL-enabled build, add in the OpenCL-specific algorithms package
set ( OPENCL_BUILD OFF CACHE BOOL "Enable building the GPUAlgorithms package using OpenCL. Requires OpenCL." )
if ( OPENCL_BUILD )
  add_subdirectory ( GPUAlgorithms )
endif ()

# Unit test helper packages 
if ( CXXTEST_FOUND )
  add_subdirectory (UserAlgorithms)
endif ()

add_subdirectory (MDAlgorithms)
add_subdirectory (MDEvents)
add_subdirectory (Doxygen)

###########################################################################
# Add a custom target to build all of the Framework
###########################################################################

set ( FRAMEWORK_LIBS Kernel Geometry API PythonAPI PythonKernelModule 
                     PythonGeometryModule PythonAPIModule DataObjects
                     DataHandling Nexus NexusCPP Algorithms CurveFitting ICat
                     Crystal MDAlgorithms MDEvents WorkflowAlgorithms
)

add_custom_target( Framework DEPENDS ${FRAMEWORK_LIBS} ) 

###########################################################################
# Installation settings
# These need to be here so that a Framework only install will pick them up
#
# N.B. INBUNDLE variable is empty except on Mac (set in DarwinSetup.cmake)
###########################################################################

# Create instrument directory and make it writable so vtp files can go there
install ( DIRECTORY ../instrument/ DESTINATION ${INBUNDLE}instrument 
          DIRECTORY_PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE
                                GROUP_READ GROUP_EXECUTE GROUP_WRITE
                                WORLD_READ WORLD_EXECUTE WORLD_WRITE
          PATTERN "*UNIT_TESTING*" EXCLUDE
)

# Scripts
install ( DIRECTORY ../scripts/ DESTINATION ${INBUNDLE}scripts PATTERN ".svn" EXCLUDE 
                                                 PATTERN "*.pyc" EXCLUDE )
