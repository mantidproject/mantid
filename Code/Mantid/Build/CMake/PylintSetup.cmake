########################################################
# Setup target to run pylint checking if it is available
########################################################
find_package ( Pylint )

if ( PYLINT_FOUND AND PYLINT_VERSION VERSION_LESS "1.0.0" )
  message ( STATUS "pylint found but version is < 1.0.0 (${PYLINT_VERSION}): no target generated." )
  set ( PYLINT_FOUND FALSE )
endif()

if ( PYLINT_FOUND )
  message ( STATUS "pylint version: ${PYLINT_VERSION}" )

  # Output format options
  if ( NOT  PYLINT_OUTPUT_FORMAT )
    # Use msvs for MSVC generator, colorized for everything else. The buildservers can
    # set it to parseable for Jenkins
    if ( MSVC )
      set ( DEFAULT_FORMAT "msvs" )
    else ()
      set ( DEFAULT_FORMAT "colorized" )
    endif ()
    set ( PYLINT_OUTPUT_FORMAT ${DEFAULT_FORMAT} CACHE STRING "Desired pylint output format" )

    # There are a limited set of allowed options
    set_property( CACHE PYLINT_OUTPUT_FORMAT PROPERTY STRINGS "text" "html" "msvs" "parseable" "colorized" )
  endif()

  # add a pylint-check target to run pylint on the required files and directories
  # relative to the root source directory
  set ( BASE_DIR ${CMAKE_SOURCE_DIR} )
  set ( PYLINT_INCLUDES
        Framework/PythonInterface/plugins
        scripts
  )
  set ( PYLINT_EXCLUDES
        scripts/lib1to2
        scripts/test
  )
  add_custom_target ( pylintcheck
                      COMMAND ${PYTHON_EXECUTABLE} ${PYLINT_RUNNER_SCRIPT} --format=${PYLINT_OUTPUT_FORMAT}
                              --rcfile=${PYLINT_CFG_FILE}
                              --mantidpath=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
                              --basedir=${BASE_DIR} --nofail
                              --exclude="${PYLINT_EXCLUDES}"
                              ${PYLINT_INCLUDES}
                      COMMENT "Running pylint on selected python files"
                    )
  set_target_properties ( pylintcheck PROPERTIES EXCLUDE_FROM_ALL TRUE )
endif ()
