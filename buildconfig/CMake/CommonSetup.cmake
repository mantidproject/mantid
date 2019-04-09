# Include useful utils
include ( MantidUtils )
include ( GenerateMantidExportHeader )
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

if ( CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR MATCHES "Xcode" )
  set ( PVPLUGINS_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/paraview )
else ()
  set ( PVPLUGINS_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/plugins/paraview )
endif()

# This allows us to group targets logically in Visual Studio
set_property ( GLOBAL PROPERTY USE_FOLDERS ON )

# Look for stdint and add define if found
include ( CheckIncludeFiles )
check_include_files ( stdint.h stdint )
if ( stdint )
  add_definitions ( -DHAVE_STDINT_H )
endif ( stdint )

# Configure a variable to hold the required test timeout value for all tests
set ( TESTING_TIMEOUT 300 CACHE INTEGER
      "Timeout in seconds for each test (default 300=5minutes)")

###########################################################################
# Look for dependencies
# Do NOT add include_directories commands here. They will affect every
# target.
###########################################################################

set ( Boost_NO_BOOST_CMAKE TRUE )
find_package ( Boost 1.53.0 REQUIRED date_time regex serialization filesystem system)
add_definitions ( -DBOOST_ALL_DYN_LINK -DBOOST_ALL_NO_LIB )
# Need this defined globally for our log time values
add_definitions ( -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG )

find_package ( Poco 1.4.6 REQUIRED )
find_package ( Nexus 4.3.1 REQUIRED )
find_package ( MuParser REQUIRED )
find_package ( JsonCPP 0.7.0 REQUIRED )

set ( ENABLE_OPENCASCADE ON CACHE BOOL "Enable OpenCascade-based 3D visualisation" )
if (ENABLE_OPENCASCADE)
  find_package ( OpenCascade REQUIRED )
  add_definitions ( -DENABLE_OPENCASCADE )
endif ()

find_package ( Doxygen ) # optional

if(CMAKE_HOST_WIN32)
  find_package ( ZLIB REQUIRED
    CONFIGS zlib-config.cmake )
  set (HDF5_DIR "${THIRD_PARTY_DIR}/cmake")
  find_package ( HDF5 COMPONENTS CXX HL REQUIRED
    CONFIGS hdf5-config.cmake )
else()
  find_package ( ZLIB REQUIRED )
  if (APPLE AND OSX_VERSION VERSION_LESS 10.9)
    set (HDF5_DIR "${CMAKE_MODULE_PATH}")
    find_package ( HDF5 COMPONENTS CXX HL REQUIRED
      CONFIGS hdf5-config.cmake )
    add_definitions ( -DH5_BUILT_AS_DYNAMIC_LIB )
  else()
    find_package ( HDF5 COMPONENTS CXX HL REQUIRED )
  endif()
endif()

find_package ( PythonInterp )
set ( Python_ADDITIONAL_VERSIONS ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} )

find_package ( OpenSSL REQUIRED )

###########################################################################
# Look for Git. Used for version headers - faked if not found.
# Also makes sure our commit hooks are linked in the right place.
###########################################################################

set ( MtdVersion_WC_LAST_CHANGED_DATE Unknown )
set ( MtdVersion_WC_LAST_CHANGED_DATETIME 0 )
set ( MtdVersion_WC_LAST_CHANGED_SHA Unknown )
set ( NOT_GIT_REPO "Not" )

if ( GIT_FOUND )
  # Get the last revision
  execute_process ( COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                    OUTPUT_VARIABLE GIT_SHA_HEAD
                    ERROR_VARIABLE NOT_GIT_REPO
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if ( NOT NOT_GIT_REPO ) # i.e This is a git repository!
    # "git describe" was originally used to produce this variable and this prefixes the short
    # SHA1 with a 'g'. We keep the same format here now that we use rev-parse
    set ( MtdVersion_WC_LAST_CHANGED_SHA "g${GIT_SHA_HEAD}" )
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
  configure_file ( ${GIT_TOP_LEVEL}/buildconfig/CMake/PatchVersionNumber.cmake.in
                   ${GIT_TOP_LEVEL}/buildconfig/CMake/PatchVersionNumber.cmake )
  include ( PatchVersionNumber )
endif()

###########################################################################
# Include the file that contains the version number
# This must come after the git business above because it can be
# used to override the patch version number
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
# Set the c++ standard to 14 - cmake should do the right thing with msvc
###########################################################################
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###########################################################################
# Add compiler options if using gcc
###########################################################################
if ( CMAKE_COMPILER_IS_GNUCXX )
  include ( GNUSetup )
elseif ( ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" )
  include ( GNUSetup )
endif ()

###########################################################################
# Configure clang-tidy if the tool is found
###########################################################################

if ( CMAKE_VERSION VERSION_GREATER "3.5" )
  set(DEFAULT_CLANG_TIDY_CHECKS "-*,performance-for-range-copy,performance-unnecessary-copy-initialization,modernize-use-override,modernize-use-nullptr,modernize-loop-convert,modernize-use-bool-literals,modernize-deprecated-headers,misc-*,-misc-unused-parameters")
  set(ENABLE_CLANG_TIDY OFF CACHE BOOL "Add clang-tidy automatically to builds")
  if (ENABLE_CLANG_TIDY)
    find_program (CLANG_TIDY_EXE NAMES "clang-tidy" PATHS /usr/local/opt/llvm/bin )
    if (CLANG_TIDY_EXE)
      message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
      set(CLANG_TIDY_CHECKS "${DEFAULT_CLANG_TIDY_CHECKS}" CACHE STR "Select checks to perform")
      option(APPLY_CLANG_TIDY_FIX "Apply fixes found through clang-tidy checks" OFF)
      if(CLANG_TIDY_CHECKS STREQUAL "")
        # use default checks if empty to avoid errors
        set(CLANG_TIDY_CHECKS "${DEFAULT_CLANG_TIDY_CHECKS}" CACHE STR "Select checks to perform" FORCE)
      endif()
      if(APPLY_CLANG_TIDY_FIX)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=${CLANG_TIDY_CHECKS};-header-filter='${CMAKE_SOURCE_DIR}/*';-fix"
          CACHE STRING "" FORCE)
      else()
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=${CLANG_TIDY_CHECKS};-header-filter='${CMAKE_SOURCE_DIR}/*'"
          CACHE STRING "" FORCE)
      endif()
    else()
      message(AUTHOR_WARNING "clang-tidy not found!")
      set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE) # delete it
    endif()
  else()
    set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE) # delete it
  endif()
else()
  message(AUTHOR_WARNING "Using cmake version 3.5 or below. Clang-tidy is not supported!")
endif()

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
  add_custom_target( check COMMAND ${CMAKE_CTEST_COMMAND} )
  make_directory( ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Testing )
  message ( STATUS "Added target ('check') for unit tests" )
else ()
  message ( STATUS "Could NOT find CxxTest - unit testing not available" )
endif ()

include ( GoogleTest )
include ( PyUnitTest )
enable_testing ()

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
# External Data for testing
###########################################################################
if ( CXXTEST_FOUND OR PYUNITTEST_FOUND )
  include( SetupDataTargets )
endif()

###########################################################################
# Visibility Setting
###########################################################################
if ( CMAKE_COMPILER_IS_GNUCXX )
  set(CMAKE_CXX_VISIBILITY_PRESET hidden CACHE STRING "")
endif()

###########################################################################
# Bundles setting used for install commands if not set by something else
# e.g. Darwin
###########################################################################
if ( NOT BUNDLES )
  set ( BUNDLES "./" )
endif()

###########################################################################
# Set a flag to indicate that this script has been called
###########################################################################

set ( COMMONSETUP_DONE TRUE )
