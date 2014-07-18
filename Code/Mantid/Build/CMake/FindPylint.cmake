# - Find Pylint
# Find the Pylint executable and extract the version number
#
# OUTPUT Variables
#
#   PYLINT_FOUND
#       True if the pylint package was found
#   PYLINT_EXECUTABLE
#       The pylint executable location
#   PYLINT_VERSION
#       A string denoting the version of pylint that has been found
#   PYLINT_RUNNER_SCRIPT
#       Location of python script that will actually run pylint
#   PYLINT_CFG_FILE
#       Location of configuration file
#

#=============================================================
# main()
#=============================================================

find_program ( PYLINT_EXECUTABLE pylint PATHS /usr/bin )

if ( PYLINT_EXECUTABLE )
  execute_process ( COMMAND ${PYLINT_EXECUTABLE} --version OUTPUT_VARIABLE PYLINT_VERSION_RAW ERROR_QUIET )
  if (PYLINT_VERSION_RAW)
    string ( REGEX REPLACE "^pylint ([0-9]+.[0-9]+.[0-9]+),.*" "\\1" PYLINT_VERSION ${PYLINT_VERSION_RAW})
  else ()
    set ( PYLINT_VERSION "unknown" )
  endif()
  # Script to find all .py files and execute pylint
  set ( PYLINT_RUNNER_SCRIPT ${CMAKE_SOURCE_DIR}/../Tools/Pylint/run_pylint.py )
  # Configuration file
  set ( PYLINT_CFG_FILE ${CMAKE_SOURCE_DIR}/../Tools/Pylint/pylint.cfg )
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS ( Pylint DEFAULT_MSG PYLINT_EXECUTABLE )

mark_as_advanced ( PYLINT_EXECUTABLE PYLINT_VERSION )
