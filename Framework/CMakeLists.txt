# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required (VERSION 3.5)

# Add the path to our custom 'find' modules
set ( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../buildconfig/CMake")

# Define the project name. Allows building framework as a separate project.
project ( MantidFramework )

# Set paths to Third_Party for Windows
if ( NOT THIRD_PARTY_DIR )
  if ( MSVC )
    include ( MSVC)
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

check_include_files ( stdint.h stdint )
if ( stdint )
  add_definitions ( -DHAVE_STDINT_H )
endif ()
check_include_files ( cstdint.hpp boost_stdint)
if ( boost_stdint )
  add_definitions ( -DBOOST_CSTDINT_HPP )
endif ()

###########################################################################
# Globally-linked libraries variable
###########################################################################

# Might just as well link everything to Boost & Poco (found in CommonSetup)
# Boost_LIBRARIES variable is redefined by MPISetup so capture the value here
# and just add MPI stuff if required
set ( MANTIDLIBS  ${Boost_LIBRARIES} ${POCO_LIBRARIES} )

###########################################################################
# MPI-enable build setup
###########################################################################

# Only available on Linux (specifically RedHat), and for a framework-only build
if ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
  set ( MPI_BUILD OFF CACHE BOOL "Enable MPI options" )
  if ( MPI_BUILD )
    include ( MPISetup )
    set ( MANTIDLIBS  ${MANTIDLIBS} ${Boost_LIBRARIES} ${MPI_CXX_LIBRARIES} )
  endif ( MPI_BUILD )
endif ()

set ( MPI_EXPERIMENTAL OFF CACHE BOOL "Enable MPI options" )
if ( MPI_EXPERIMENTAL )
  include ( BoostMPI )
endif ()

###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################

add_custom_target ( FrameworkTests ) # target for all framework tests
add_dependencies ( check FrameworkTests )

include_directories (Types/inc)
add_subdirectory (Types)

include_directories (Kernel/inc)
add_subdirectory (Kernel)

include_directories (Parallel/inc)
add_subdirectory (Parallel)

include_directories (HistogramData/inc)
add_subdirectory (HistogramData)

include_directories (Indexing/inc)
add_subdirectory (Indexing)

include_directories (Beamline/inc)
add_subdirectory (Beamline)

set ( MANTIDLIBS ${MANTIDLIBS} Parallel )
# HistogramData has header-only dependency on Kernel, so Kernel comes after.
set ( MANTIDLIBS ${MANTIDLIBS} HistogramData )
# Indexing has header-only dependency on Kernel, so Kernel comes after.
set ( MANTIDLIBS ${MANTIDLIBS} Indexing )
set ( MANTIDLIBS ${MANTIDLIBS} Kernel )
set ( MANTIDLIBS ${MANTIDLIBS} Beamline )

include_directories (Geometry/inc)
# muParser needed by Geometry and subsequent packages
include_directories ( ${MUPARSER_INCLUDE_DIR} )
set ( MANTIDLIBS ${MANTIDLIBS} ${MUPARSER_LIBRARIES} )
add_subdirectory (Geometry)
set ( MANTIDLIBS ${MANTIDLIBS} Geometry )

include_directories (API/inc)
add_subdirectory (API)
set ( MANTIDLIBS ${MANTIDLIBS} API )

add_subdirectory (PythonInterface)

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
add_subdirectory (LiveData)
add_subdirectory (RemoteAlgorithms)
add_subdirectory (RemoteJobManagers)
add_subdirectory (SINQ)

# If an MPI-enabled build, add in the MPI-specific algorithms package
if ( MPI_BUILD )
  add_subdirectory ( MPIAlgorithms )
endif ()

# If a OpenCL-enabled build, add in the OpenCL-specific algorithms package
set ( OPENCL_BUILD OFF CACHE BOOL "Enable building the GPUAlgorithms package using OpenCL. Requires OpenCL." )
if ( OPENCL_BUILD )
  add_subdirectory ( GPUAlgorithms )
endif ()

add_subdirectory (MDAlgorithms)
add_subdirectory (Doxygen)
add_subdirectory (ScriptRepository)

###########################################################################
# Add a custom target to build all of the Framework
###########################################################################

set ( FRAMEWORK_LIBS Kernel HistogramData Indexing Beamline Geometry API DataObjects
                     PythonKernelModule PythonGeometryModule PythonAPIModule
                     PythonDataObjectsModule
                     DataHandling Nexus Algorithms CurveFitting ICat
                     Crystal MDAlgorithms WorkflowAlgorithms
                     LiveData RemoteAlgorithms RemoteJobManagers
                     SINQ
)

add_custom_target( Framework DEPENDS ${FRAMEWORK_LIBS} )

###########################################################################
# Installation settings
# These need to be here so that a Framework only install will pick them up
#
# N.B. INBUNDLE variable is empty except on Mac (set in DarwinSetup.cmake)
###########################################################################

# Create instrument directory
install ( DIRECTORY ../instrument/ DESTINATION ${INBUNDLE}instrument
          PATTERN "*UNIT_TESTING*" EXCLUDE
          PATTERN ".gitignore" EXCLUDE
)

# Ships .py files but only ship compiled pyd files for supported platforms.
if ( WIN32 ) # General windows environment
  if ( CMAKE_SIZEOF_VOID_P EQUAL 8 ) # Recommended way of detecting 64- vs 32-bit build
    # Excludes .so files & _win32 binaries
    install ( DIRECTORY ../scripts/ DESTINATION ${INBUNDLE}scripts PATTERN "*.pyc"
              EXCLUDE PATTERN ".svn" EXCLUDE PATTERN ".gitignore"
              EXCLUDE PATTERN "*.so" EXCLUDE PATTERN "*_win32.pyd" EXCLUDE PATTERN "CMakeLists.txt"
              EXCLUDE PATTERN "test" EXCLUDE )
  else ()
    # Excludes so files & _win64 binaries
    install ( DIRECTORY ../scripts/ DESTINATION ${INBUNDLE}scripts PATTERN "*.pyc"
              EXCLUDE PATTERN ".svn" EXCLUDE PATTERN ".gitignore"
              EXCLUDE PATTERN "*.so" EXCLUDE PATTERN "*_win64.pyd" EXCLUDE PATTERN "CMakeLists.txt"
              EXCLUDE PATTERN "test" EXCLUDE )
  endif ()

  # Also ship mingw libraries for Inelastic fortran code. We need to do a better job here and build things
  file ( GLOB MINGW_DLLS "${THIRD_PARTY_DIR}/bin/mingw/*.dll" )
  install ( FILES ${MINGW_DLLS} DESTINATION ${INBUNDLE}scripts/Inelastic )
else ()
  # These don't work correctly and the linux ones are in no way general. They really need to be part of the build
  install ( DIRECTORY ../scripts/ DESTINATION ${INBUNDLE}scripts PATTERN "*.pyc"
            EXCLUDE PATTERN ".svn" EXCLUDE PATTERN ".gitignore"
            EXCLUDE PATTERN "*_win*.pyd" EXCLUDE PATTERN "*_lnx64.so" EXCLUDE PATTERN "CMakeLists.txt"
            EXCLUDE PATTERN "test" EXCLUDE )
endif ()

# THIS MUST BE THE LAST SUB_DIRECTORY ADDED. See Properties/CMakeLists.txt.
# This is included by the top-level CMakeLists if it is a full build but do it here for a Framework only
if( NOT FULL_PACKAGE_BUILD )
  add_subdirectory( PostInstall )
endif()
