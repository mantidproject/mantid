# Print a warning about ctest if using v2.6
if ( CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION LESS 8 )
  message ( WARNING " Running tests via CTest will not work with this version of CMake. If you need this functionality, upgrade to CMake 2.8." )
endif ()

# Include useful utils
include ( MantidUtils )

# Make the default build type Release
if ( NOT CMAKE_CONFIGURATION_TYPES )
  if ( NOT CMAKE_BUILD_TYPE )
    message ( STATUS "No build type selected, default to Release." )
    set( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE )
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
      "MinSizeRel" "RelWithDebInfo")
  else ()
    message ( STATUS "Build type is " ${CMAKE_BUILD_TYPE} )
  endif ()
endif()

# We want shared libraries everywhere
set ( BUILD_SHARED_LIBS On )

# Send libraries to common place
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )

# This allows us to group targets logically in Visual Studio
set_property ( GLOBAL PROPERTY USE_FOLDERS ON )

# Look for stdint and add define if found
include ( CheckIncludeFiles )
check_include_files ( stdint.h stdint )
if ( stdint )
  add_definitions ( -DHAVE_STDINT_H )
endif ( stdint )

###########################################################################
# Include the file that contains the minor (iteration) version number
###########################################################################

include ( VersionNumber )

###########################################################################
# Look for dependencies - bail out if any not found
###########################################################################

find_package ( Boost REQUIRED signals date_time regex )
include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
add_definitions ( -DBOOST_ALL_DYN_LINK )
# Need this defined globally for our log time values
add_definitions ( -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG )

find_package ( Poco REQUIRED )
include_directories( SYSTEM ${POCO_INCLUDE_DIRS} )

find_package ( Nexus REQUIRED )
include_directories ( SYSTEM ${NEXUS_INCLUDE_DIR} )

find_package ( MuParser REQUIRED )

# Need to change search path to find zlib include on Windows.
# Couldn't figure out how to extend CMAKE_INCLUDE_PATH variable for extra path
# so I'm caching old value, changing it temporarily and then setting it back
set ( MAIN_CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} )
set ( CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}/zlib123 )
find_package ( ZLIB REQUIRED )
set ( CMAKE_INCLUDE_PATH ${MAIN_CMAKE_INCLUDE_PATH} )

###########################################################################
# Look for Git. Used for version headers - faked if not found.
# Also makes sure our commit hooks are linked in the right place.
###########################################################################

set ( MtdVersion_WC_LAST_CHANGED_REV 0 )
set ( MtdVersion_WC_LAST_CHANGED_DATE Unknown )
set ( NOT_GIT_REPO "Not" )

find_package ( Git )
if ( GIT_FOUND )
  # Get the last revision
  execute_process ( COMMAND ${GIT_EXECUTABLE} describe --tags --long 
                    OUTPUT_VARIABLE GIT_DESCRIBE
                    ERROR_VARIABLE NOT_GIT_REPO
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
  if ( NOT NOT_GIT_REPO ) # i.e This is a git repository!

    # Remove the tag name part
    string ( REGEX MATCH "[-](.*)" MtdVersion_WC_LAST_CHANGED_REV ${GIT_DESCRIBE} )
    # Extract the SHA1 part (with a 'g' prefix which stands for 'git')
    # N.B. The variable comes back from 'git describe' with a line feed on the end, so we need to lose that
    string ( REGEX MATCH "(g.*)[^\n]" MtdVersion_WC_LAST_CHANGED_SHA ${MtdVersion_WC_LAST_CHANGED_REV} )
    # Get the number part (number of commits since tag)
    string ( REGEX MATCH "[0-9]+" MtdVersion_WC_LAST_CHANGED_REV ${MtdVersion_WC_LAST_CHANGED_REV} )
    # Get the date of the last commit
    execute_process ( COMMAND ${GIT_EXECUTABLE} log -1 --format=format:%cD OUTPUT_VARIABLE MtdVersion_WC_LAST_CHANGED_DATE 
                      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    string ( SUBSTRING ${MtdVersion_WC_LAST_CHANGED_DATE} 0 16 MtdVersion_WC_LAST_CHANGED_DATE )

    ###########################################################################
    # This part puts our hooks (in .githooks) into .git/hooks
    ###########################################################################
    # First need to find the top-level directory of the git repository
    execute_process ( COMMAND ${GIT_EXECUTABLE} rev-parse --show-toplevel
                      OUTPUT_VARIABLE GIT_TOP_LEVEL
                      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    # N.B. The variable comes back from 'git describe' with a line feed on the end, so we need to lose that
    string ( REGEX MATCH "(.*)[^\n]" GIT_TOP_LEVEL ${GIT_TOP_LEVEL} )
    # Prefer symlinks on platforms that support it so we don't rely on cmake running to be up-to-date
    # On Windows, we have to copy the file
    if ( WIN32 )
      execute_process ( COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GIT_TOP_LEVEL}/.githooks/pre-commit
                                                                      ${GIT_TOP_LEVEL}/.git/hooks )
      execute_process ( COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GIT_TOP_LEVEL}/.githooks/commit-msg
                                                                      ${GIT_TOP_LEVEL}/.git/hooks )
    else ()
      execute_process ( COMMAND ${CMAKE_COMMAND} -E create_symlink ${GIT_TOP_LEVEL}/.githooks/pre-commit
                                                                   ${GIT_TOP_LEVEL}/.git/hooks/pre-commit )
      execute_process ( COMMAND ${CMAKE_COMMAND} -E create_symlink ${GIT_TOP_LEVEL}/.githooks/commit-msg
                                                                   ${GIT_TOP_LEVEL}/.git/hooks/commit-msg )
    endif ()

  endif()

else()
  # Just use a dummy version number and print a warning
  message ( STATUS "Git not found - using dummy revision number and date" )
endif()

mark_as_advanced( MtdVersion_WC_LAST_CHANGED_REV MtdVersion_WC_LAST_CHANGED_DATE )

###########################################################################
# Look for OpenMP and set compiler flags if found
###########################################################################

find_package ( OpenMP )

###########################################################################
# Add linux-specific things
###########################################################################
if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  include ( LinuxSetup )
endif ()

###########################################################################
# Add compiler options if using gcc
###########################################################################
if ( CMAKE_COMPILER_IS_GNUCXX )
  include ( GNUSetup )
endif ()

###########################################################################
# Set up the unit tests target
###########################################################################

find_package ( CxxTest )
if ( CXXTEST_FOUND )
  include_directories ( ${CXXTEST_INCLUDE_DIR} )
  enable_testing ()
  add_custom_target( check COMMAND ${CMAKE_CTEST_COMMAND} )
  make_directory( ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Testing )
  message ( STATUS "Added target ('check') for unit tests" )
else ()
  message ( STATUS "Could NOT find CxxTest - unit testing not available" )
endif ()

# Some unit tests need GMock/GTest
find_package ( GMock )

if ( GMOCK_FOUND AND GTEST_FOUND )
  include_directories ( SYSTEM ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )
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
# Allow binaries to be put into subdirectories.
###########################################################################
include(OutputBinaryTo)

###########################################################################
# Set a flag to indicate that this script has been called
###########################################################################

set ( COMMONSETUP_DONE TRUE )
