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
# Look for dependencies - bail out if any not found
###########################################################################

set ( Boost_NO_BOOST_CMAKE TRUE )
find_package ( Boost REQUIRED date_time regex ) 
include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
add_definitions ( -DBOOST_ALL_DYN_LINK )
# Need this defined globally for our log time values
add_definitions ( -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG )

find_package ( Poco REQUIRED )
include_directories( SYSTEM ${POCO_INCLUDE_DIRS} )

find_package ( Nexus 4.3.0 REQUIRED )
include_directories ( SYSTEM ${NEXUS_INCLUDE_DIR} )

find_package ( MuParser REQUIRED )

find_package ( Doxygen ) # optional

# Need to change search path to find zlib include on Windows.
# Couldn't figure out how to extend CMAKE_INCLUDE_PATH variable for extra path
# so I'm caching old value, changing it temporarily and then setting it back
set ( MAIN_CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} )
set ( CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}/zlib123 )
find_package ( ZLIB REQUIRED )
set ( CMAKE_INCLUDE_PATH ${MAIN_CMAKE_INCLUDE_PATH} )

find_package ( PythonInterp )

if ( MSVC )
  # Wrapper script to call either python or python_d depending on directory contents
  set ( PYTHON_EXE_WRAPPER_SRC "${CMAKE_MODULE_PATH}/../win_python.bat" )
  set ( PYTHON_EXE_WRAPPER "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/win_python.bat" )
else()
  set ( PYTHON_EXE_WRAPPER ${PYTHON_EXECUTABLE} )
endif()

###########################################################################
# Look for Git. Used for version headers - faked if not found.
# Also makes sure our commit hooks are linked in the right place.
###########################################################################

set ( MtdVersion_WC_LAST_CHANGED_DATE Unknown )
set ( MtdVersion_WC_LAST_CHANGED_DATETIME 0 )
set ( NOT_GIT_REPO "Not" )

find_package ( Git )
if ( GIT_FOUND )
  # Get the last revision
  execute_process ( COMMAND ${GIT_EXECUTABLE} describe --long
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

    # Get the date of the last commit
    execute_process ( COMMAND ${GIT_EXECUTABLE} log -1 --format=format:%cD OUTPUT_VARIABLE MtdVersion_WC_LAST_CHANGED_DATE 
                      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    string ( SUBSTRING ${MtdVersion_WC_LAST_CHANGED_DATE} 0 16 MtdVersion_WC_LAST_CHANGED_DATE )

    execute_process ( COMMAND ${GIT_EXECUTABLE} log -1 --format=format:%H
                      OUTPUT_VARIABLE MtdVersion_WC_LAST_CHANGED_SHA_LONG
                      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

    # getting the datetime (as iso8601 string) to turn into the patch string
    execute_process ( COMMAND ${GIT_EXECUTABLE} log -1 --format=format:%ci
                      OUTPUT_VARIABLE MtdVersion_WC_LAST_CHANGED_DATETIME
                      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    if ( MtdVersion_WC_LAST_CHANGED_DATETIME )
      # split into "date time timezone"
      string (REPLACE " " ";" LISTVERS ${MtdVersion_WC_LAST_CHANGED_DATETIME})
      list (GET LISTVERS 0 ISODATE)
      list (GET LISTVERS 1 ISOTIME)
      list (GET LISTVERS 2 ISOTIMEZONE)

      # turn the date into a number
      string (REPLACE "-" "" ISODATE ${ISODATE})

      # prepare the time
      string (REGEX REPLACE "^([0-9]+:[0-9]+).*" "\\1" ISOTIME ${ISOTIME})
      string (REPLACE ":" "" ISOTIME ${ISOTIME})

      # convert the timezone into something that can be evaluated for math
      if (ISOTIMEZONE STREQUAL "+0000")
        set (ISOTIMEZONE "") # GMT do nothing
      else ()
        string( SUBSTRING ${ISOTIMEZONE} 0 1 ISOTIMEZONESIGN)
	if (ISOTIMEZONESIGN STREQUAL "+")
	  string (REPLACE "+" "-" ISOTIMEZONE ${ISOTIMEZONE})
        else ()
	  string (REPLACE "-" "+" ISOTIMEZONE ${ISOTIMEZONE})
	endif()
      endif ()

      # remove the timezone from the time to convert to GMT
      math (EXPR ISOTIME "${ISOTIME}${ISOTIMEZONE}" )

      # deal with times crossing midnight
      # this does not get the number of days in a month right or jan 1st/dec 31st
      if ( ISOTIME GREATER 2400 )
          math (EXPR ISOTIME "${ISOTIME}-2400" )
	  math (EXPR ISODATE "${ISODATE}+1")
      elseif (ISOTIME LESS 0)
          math (EXPR ISOTIME "2400${ISOTIME}" )
	  math (EXPR ISODATE "${ISODATE}-1")
      endif ()

      set (MtdVersion_WC_LAST_CHANGED_DATETIME "${ISODATE}.${ISOTIME}")
    endif ()

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

mark_as_advanced( MtdVersion_WC_LAST_CHANGED_DATE MtdVersion_WC_LAST_CHANGED_DATETIME )


if ( NOT NOT_GIT_REPO ) # i.e This is a git repository!
  ###########################################################################
  # Create the file containing the patch version number for use by cpack
  # The patch number make have been overridden by VersionNumber so create
  # the file used by cpack here
  ###########################################################################
  configure_file ( ${GIT_TOP_LEVEL}/Code/Mantid/Build/CMake/PatchVersionNumber.cmake.in
                   ${GIT_TOP_LEVEL}/Code/Mantid/Build/CMake/PatchVersionNumber.cmake )
  include ( PatchVersionNumber )
endif()

###########################################################################
# Include the file that contains the version number
# This must come after the git describe business above because it can be
# used to override the patch version number (MtdVersion_WC_LAST_CHANGED_REV)
###########################################################################
include ( VersionNumber )


###########################################################################
# Look for OpenMP and set compiler flags if found
###########################################################################

find_package ( OpenMP )
if ( OPENMP_FOUND )
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}" )
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}" )
  if ( NOT WIN32 )
    set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${OpenMP_CXX_FLAGS}" )
  endif ()
endif ()


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
# Setup cppcheck
###########################################################################
include ( CppCheckSetup )

###########################################################################
# Setup pylint
###########################################################################
include ( PylintSetup )

###########################################################################
# Set up the unit tests target
###########################################################################

find_package ( CxxTest )
if ( CXXTEST_FOUND )
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
  message ( STATUS "GMock/GTest (${GMOCK_VERSION}) is available for unit tests." )
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

# GUI testing via Squish
find_package ( Squish )
if ( SQUISH_FOUND )
  # CMAKE_MODULE_PATH gets polluted when ParaView is present
  set( MANTID_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} )
  include( SquishAddTestSuite )
  enable_testing()
  message ( STATUS "Found Squish for GUI testing" )
else()
  message ( STATUS "Could not find Squish - GUI testing not available. Try specifying your SQUISH_INSTALL_DIR cmake variable." )
endif()

###########################################################################
# Set a flag to indicate that this script has been called
###########################################################################

set ( COMMONSETUP_DONE TRUE )
