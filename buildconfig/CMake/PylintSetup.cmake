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
  # Use msvs for MSVC generator, colorized for everything else. The buildservers can
  # set it to parseable for Jenkins
  if ( MSVC )
    set ( DEFAULT_FORMAT "msvs" )
  else ()
    set ( DEFAULT_FORMAT "{C}:{line:3d},{column:2d}: {msg} ({symbol})" )
  endif ()
  set ( PYLINT_MSG_TEMPLATE ${DEFAULT_FORMAT} CACHE STRING "Desired pylint output format" )

  # add a pylint-check target to run pylint on the required files and directories
  # relative to the root source directory
  set ( BASE_DIR ${CMAKE_SOURCE_DIR} )
  set ( PYLINT_INCLUDES
        Framework/PythonInterface/plugins
        scripts
        Testing/SystemTests/tests/analysis
        tools
        docs/sphinxext/mantiddoc
  )
  set ( PYLINT_EXCLUDES
        scripts/test
        Testing/SystemTests/tests/analysis/reference
  )

  set ( PYLINT_OUTPUT_DIR "" CACHE STRING "Directory used to write the output from each pyint check." )
  # Choose the number of threads used
  set ( PYLINT_NTHREADS 8 CACHE STRING "Number of processes used for running pylint" )

  add_custom_target ( pylintcheck
                      COMMAND ${PYTHON_EXECUTABLE} ${PYLINT_RUNNER_SCRIPT} --format="${PYLINT_MSG_TEMPLATE}"
                              --output=${PYLINT_OUTPUT_DIR}
                              --rcfile=${PYLINT_CFG_FILE}
                              --mantidpath=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
                              --basedir=${BASE_DIR} --nofail
                              --exclude="${PYLINT_EXCLUDES}"
                              --parallel=${PYLINT_NTHREADS}
                              ${PYLINT_INCLUDES}
                      COMMENT "Running pylint on selected python files"
                    )
  set_target_properties ( pylintcheck PROPERTIES EXCLUDE_FROM_ALL TRUE )
endif ()
