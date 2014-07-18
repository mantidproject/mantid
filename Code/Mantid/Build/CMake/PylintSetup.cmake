########################################################
# Setup target to run pylint checking if it is available
########################################################
find_package ( Pylint )

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

  # add a pylint-check target to run pylint on all discoverable .py files
  # starting in the root of the code directory
  set ( PYLINT_START_DIR ${CMAKE_SOURCE_DIR} )
  add_custom_target ( pylint-check 
                      COMMAND ${PYTHON_EXECUTABLE} ${PYLINT_RUNNER_SCRIPT} --format=${PYLINT_OUTPUT_FORMAT}
                              --rcfile=${PYLINT_CFG_FILE} ${PYLINT_START_DIR}
                      COMMENT "Running pylint on all python files"
                    )
  set_target_properties ( pylint-check PROPERTIES EXCLUDE_FROM_ALL TRUE )
endif ()
