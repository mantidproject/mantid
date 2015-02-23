# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required (VERSION 2.8.12)

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

option(BUILD_MATLAB_DEPENDENCIES "Build Mantid-Matlab interface in the case of Matlab was found" OFF)
###########################################################################
# Look for dependencies - bail out if any necessary ones not found
###########################################################################

# gsl is currently needed by Geometry, Algorithms & Curvefitting
find_package ( GSL REQUIRED )

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
if ( ${CMAKE_PROJECT_NAME} MATCHES "MantidFramework" AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
  set ( MPI_BUILD OFF CACHE BOOL "Enable MPI options" )
  if ( MPI_BUILD )
  include ( MPISetup )
        set ( MANTIDLIBS  ${MANTIDLIBS} ${Boost_LIBRARIES} ${MPI_CXX_LIBRARIES} )
  endif ( MPI_BUILD )
endif ()

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

add_subdirectory (PythonInterface)
#
if (BUILD_MATLAB_DEPENDENCIES)
    find_package ( Matlab )
    if( MATLAB_FOUND )
        add_subdirectory (MatlabAPI)
    endif ()
endif()

include_directories (DataObjects/inc)
add_subdirectory (DataObjects)
set ( MANTIDLIBS ${MANTIDLIBS} DataObjects )

add_subdirectory (RemoteAlgorithms)
add_subdirectory (Nexus)
add_subdirectory (DataHandling)
add_subdirectory (Algorithms)
add_subdirectory (WorkflowAlgorithms)
add_subdirectory (CurveFitting)
add_subdirectory (Crystal)
add_subdirectory (ICat)
add_subdirectory (LiveData)
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

# Unit test helper packages
if ( CXXTEST_FOUND )
  add_subdirectory ( UserAlgorithms )
  # This needs to be here so that a Framework-only build will work.
#  add_subdirectory ( ../TestingTools ${${CMAKE_PROJECT_NAME}_BINARY_DIR}/TestingTools/gmock-${GMOCK_VERSION} )
  add_subdirectory ( ../TestingTools ${${CMAKE_PROJECT_NAME}_BINARY_DIR}/TestingTools )
endif ()

add_subdirectory (MDAlgorithms)
add_subdirectory (MDEvents)
add_subdirectory (Doxygen)
add_subdirectory (ScriptRepository)
add_subdirectory (ISISLiveData)

###########################################################################
# Add a custom target to build all of the Framework
###########################################################################

set ( FRAMEWORK_LIBS Kernel Geometry API PythonKernelModule
                     PythonGeometryModule PythonAPIModule DataObjects
                     DataHandling Nexus Algorithms CurveFitting ICat
                     Crystal MDAlgorithms MDEvents WorkflowAlgorithms
                     LiveData ISISLiveData RemoteAlgorithms SINQ
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

# Ships .py files but only ship compiled pyd files for supported platforms.
if ( WIN32 ) # General windows environment
  if ( CMAKE_SIZEOF_VOID_P EQUAL 8 ) # Recommended way of detecting 64- vs 32-bit build
    # Excludes .so files & _win32 binaries
    install ( DIRECTORY ../scripts/ DESTINATION ${INBUNDLE}scripts PATTERN "*.pyc" EXCLUDE
              PATTERN ".svn" EXCLUDE PATTERN "*.so" EXCLUDE PATTERN "*_win32.pyd" EXCLUDE )
  else ()
    # Excludes so files & _win64 binaries
    install ( DIRECTORY ../scripts/ DESTINATION ${INBUNDLE}scripts PATTERN "*.pyc" EXCLUDE
              PATTERN ".svn" EXCLUDE PATTERN "*.so" EXCLUDE PATTERN "*_win64.pyd" EXCLUDE )
  endif ()
    # Also ship mingw libraries for Inelastic fortran code. We need to do a better job here and build things
    file ( GLOB MINGW_DLLS "${CMAKE_LIBRARY_PATH}/mingw/*.dll" )
    install ( FILES ${MINGW_DLLS} DESTINATION ${INBUNDLE}scripts/Inelastic )
  else ()
  # These don't work correctly and the linux ones are in no way general. They really need to be part of the build
  install ( DIRECTORY ../scripts/ DESTINATION ${INBUNDLE}scripts PATTERN "*.pyc" EXCLUDE
            PATTERN ".svn" EXCLUDE PATTERN "*_win*.pyd" EXCLUDE PATTERN "*_lnx64.so" EXCLUDE )
endif ()

# THIS MUST BE THE LAST SUB_DIRECTORY ADDED. See Properties/CMakeLists.txt.
# This is included by the top-level CMakeLists if it is a full build but do it here for a Framework only
if( NOT FULL_PACKAGE_BUILD )
  add_subdirectory( PostInstall )
endif()
