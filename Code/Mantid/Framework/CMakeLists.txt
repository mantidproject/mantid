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
# This section deals with creating the MantidVersion.h header
###########################################################################

if ( Subversion_FOUND )
  # extract working copy information for SOURCE_DIR into Framework_XXX variables
  Subversion_WC_INFO(${PROJECT_SOURCE_DIR} Framework)
else ()
  # Just use a dummy version number and print a warning
  message ( "Subversion not found - using 0 for revision number in MantidVersion.h" )
  set ( Framework_WC_REVISION 0 )
endif ()
# Create the header with the revision number in
set ( HEADER_DIR Kernel/inc/MantidKernel )
configure_file ( ${PROJECT_SOURCE_DIR}/${HEADER_DIR}/MantidVersion.h.in
                 ${PROJECT_SOURCE_DIR}/${HEADER_DIR}/MantidVersion.h 
)

###########################################################################
# Look for dependencies - bail out if any necessary oned not found
###########################################################################

# Look for tcmalloc. Make it optional, for now at least, but on by default
set ( USE_TCMALLOC ON CACHE BOOL "Flag for replacing regular malloc with tcmalloc" )
# Note that this is not mandatory, so no REQUIRED
find_package ( Tcmalloc )
# If not found, or not wanted, just carry on without it
if ( USE_TCMALLOC AND TCMALLOC_FOUND )
  set ( TCMALLOC_LIBRARY ${TCMALLOC_LIBRARIES} )
  # Make a C++ define to use as flags in, e.g. MemoryManager.cpp
  add_definitions ( -DUSE_TCMALLOC )
endif ()

# Might just as well link everything to Boost & Poco (found in CommonSetup)
set ( MANTIDLIBS  ${Boost_LIBRARIES} ${POCO_LIBRARIES} ${TCMALLOC_LIBRARY} )

# gsl is currently needed by Geometry, Algorithms & Curvefitting
find_package ( GSL REQUIRED )

###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################

add_custom_target ( FrameworkTests ) # target for all framework tests

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

add_subdirectory (DataHandling)
add_subdirectory (Nexus)
add_subdirectory (Algorithms)
add_subdirectory (CurveFitting)
add_subdirectory (ICat)

# Unit test helper packages 
if ( CXXTEST_FOUND )
  add_subdirectory (TestHelpers)
  add_subdirectory (UserAlgorithms)
endif ()

if ( MAKE_VATES )
  # set hdf5 include directories as a global variable to define it later
  set ( HDF5_INCLUDE_DIRS "")
  
  add_subdirectory (MDDataObjects)
  set(MANTIDLIBS ${MANTIDLIBS} MDDataObjects)
  include_directories(MDDataObjects/inc)
  
  add_subdirectory (MDAlgorithms)
  set(MANTIDLIBS ${MANTIDLIBS} MDAlgorithms) 
  include_directories(MDAlgorithms/inc) 

  add_subdirectory (MDEvents)
  set(MANTIDLIBS ${MANTIDLIBS} MDEvents) 
  include_directories(MDEvents/inc) 
endif ()

###########################################################################
# Add a custom target to build all of the Framework
###########################################################################

set ( FRAMEWORK_LIBS Kernel Geometry API PythonAPI DataObjects
                     DataHandling Nexus Algorithms CurveFitting ICat
)

if ( MAKE_VATES )
  set ( FRAMEWORK_LIBS ${FRAMEWORK_LIBS} MDDataObjects MDAlgorithms MDEvents)
endif ()

add_custom_target ( Framework DEPENDS ${FRAMEWORK_LIBS} )
