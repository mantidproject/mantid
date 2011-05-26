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

set ( MPI_BUILD OFF CACHE BOOL "Enable MPI options" )
# Create the variable 
set ( MPI_LIBRARIES )
if ( MPI_BUILD )
  # This find OpenMPI on rhel6 (assuming it's installed, of course)
  find_package ( MPI REQUIRED )
  # Set things up to use the OpenMPI 'wrapper' compilers
  INCLUDE(CMakeForceCompiler)
  CMAKE_FORCE_C_COMPILER(mpicc "MPI C Compiler")
  CMAKE_FORCE_CXX_COMPILER(mpicxx "MPI C++ Compiler")
  include_directories ( SYSTEM ${MPI_INCLUDE_PATH} )

  set ( BOOST_ROOT /usr/lib64/openmpi ) # This is where openmpi winds up on rhel6
  find_package ( Boost REQUIRED mpi serialization )
  include_directories( ${Boost_INCLUDE_DIRS} )
  
  add_definitions ( -DMPI_BUILD )
endif ()

###########################################################################
# Globally-linked libraries variable
###########################################################################

# Might just as well link everything to Boost & Poco (found in CommonSetup)
# TCMalloc & MPI variables will be empty if not using
set ( MANTIDLIBS  ${Boost_LIBRARIES} ${POCO_LIBRARIES} ${TCMALLOC_LIBRARY} ${MPI_LIBRARIES} )

###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################

add_custom_target ( FrameworkTests ) # target for all framework tests
add_dependencies ( check FrameworkTests )

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
add_subdirectory (CurveFitting)
add_subdirectory (Crystal)
add_subdirectory (ICat)

# If an MPI-enabled build, add in the MPI-specific algorithms package
if ( MPI_BUILD )
  add_subdirectory ( MPIAlgorithms )
endif ()

# Unit test helper packages 
if ( CXXTEST_FOUND )
  add_subdirectory (TestHelpers)
  add_subdirectory (UserAlgorithms)
endif ()

add_subdirectory (MDDataObjects)
include_directories(MDDataObjects/inc)
set(MANTIDLIBS ${MANTIDLIBS} MDDataObjects)
add_subdirectory (MDAlgorithms)

# As of May 15, 2011, MDEvents is not an optional flag and is included in the main build.
add_subdirectory (MDEvents)

###########################################################################
# Add a custom target to build all of the Framework
###########################################################################

set ( FRAMEWORK_LIBS Kernel Geometry API PythonAPI DataObjects
                     DataHandling Nexus Algorithms CurveFitting ICat MDEvents
)

if ( MAKE_MULTIDIMENSIONAL )
  set ( FRAMEWORK_LIBS ${FRAMEWORK_LIBS} MDDataObjects MDAlgorithms )
endif ()

add_custom_target ( Framework DEPENDS ${FRAMEWORK_LIBS})


