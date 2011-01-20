# Print a warning about ctest if using v2.6
if ( CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION LESS 8 )
  message ( WARNING " Running tests via CTest will not work with this version of CMake. If you need this functionality, upgrade to CMake 2.8." )
endif ()

# Make the default build type Release
if ( NOT CMAKE_CONFIGURATION_TYPES )
  if ( NOT CMAKE_BUILD_TYPE )
    message ( STATUS "No build type selected, default to Release." )
    set ( CMAKE_BUILD_TYPE "Release" )
  else ()
    message ( STATUS "Build type is " ${CMAKE_BUILD_TYPE} )
  endif ()
endif()

# We want shared libraries everywhere
set ( BUILD_SHARED_LIBS On )

# Send libraries to common place
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )

# This allows us to group targets logically in Visual Studio
set_property ( GLOBAL PROPERTY USE_FOLDERS ON )

# Look for stdint and add define if found
include ( CheckIncludeFiles )
check_include_files ( stdint.h stdint )
if ( stdint )
  add_definitions ( -DHAVE_STDINT_H )
endif ( stdint )

###########################################################################
# Look for dependencies - bail out if any not found
###########################################################################

find_package ( Boost REQUIRED signals date_time regex )
include_directories( ${Boost_INCLUDE_DIRS} )
add_definitions ( -DBOOST_ALL_DYN_LINK )
# Need this defined globally for our log time values
add_definitions ( -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG )

find_package ( Poco REQUIRED )
include_directories( ${POCO_INCLUDE_DIRS} )

find_package ( MuParser REQUIRED )

# Need to change search path to find zlib include on Windows.
# Couldn't figure out how to extend CMAKE_INCLUDE_PATH variable for extra path
# so I'm caching old value, changing it temporarily and then setting it back
set ( MAIN_CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} )
set ( CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}/zlib123 )
find_package ( ZLIB REQUIRED )
set ( CMAKE_INCLUDE_PATH ${MAIN_CMAKE_INCLUDE_PATH} )

find_package ( PythonLibs REQUIRED )
include_directories ( ${PYTHON_INCLUDE_PATH} )
# If found, need to add debug library into libraries variable
if ( PYTHON_DEBUG_LIBRARIES )
  set ( PYTHON_LIBRARIES optimized ${PYTHON_LIBRARIES} debug ${PYTHON_DEBUG_LIBRARIES} )
endif ()

###########################################################################
# Look for subversion. Used for version headers - faked if not found.
###########################################################################

set ( SVN_WORKING_COPY ON CACHE BOOL "Switch off if not building off a Subversion working copy")
if ( SVN_WORKING_COPY )
  include( FindSubversion )
endif ()

###########################################################################
# Set up the unit tests target
###########################################################################

find_package ( CxxTest )
if ( CXXTEST_FOUND )
  include_directories ( ${CXXTEST_INCLUDE_DIR} )
  enable_testing ()
  add_custom_target( check COMMAND ${CMAKE_CTEST_COMMAND} )
  message ( STATUS "Added target ('check') for unit tests" )
else ()
  message ( STATUS "Could NOT find CxxTest - unit testing not available" )
endif ()

# Some unit tests need GMock/GTest
find_package ( GMock )
find_package ( GTest )
if ( GMOCK_FOUND AND GTEST_FOUND )
  include_directories ( ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )
  message ( STATUS "GMock/GTest is available for unit tests." )
else ()
  message ( STATUS "GMock/GTest is not available. Some unit tests will not run." ) 
endif()

find_package ( PyUnitTest )
if ( PYUNITTEST_FOUND )
  enable_testing ()
  message (STATUS "Found pyunittest generator")
else()
  message (STATUS "Could NOT find PyUnitTest - unit testing of python not available" )
endif()

###########################################################################
# Set a flag to indicate that this script has been called
###########################################################################

set ( COMMONSETUP_DONE TRUE )
