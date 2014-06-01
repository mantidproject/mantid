###############################################################################
# Sphinx documentation
###############################################################################
find_package ( Sphinx )

if ( SPHINX_FOUND )
  # Fill in the config file and autogen file with build information
  
  # We generate a target per build type, i.e docs-html
  set ( SPHINX_BUILD ${CMAKE_BINARY_DIR}/docs )
  set ( SCREENSHOTS_DIR ${SPHINX_BUILD}/screenshots )

  # targets
  set ( TARGET_PREFIX docs)

  # HTML target
  set ( BUILDER html )
  configure_file ( runsphinx.py.in runsphinx_html.py @ONLY )
  add_custom_target ( ${TARGET_PREFIX}-html
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/MantidPlot -xq runsphinx_html.py
    COMMENT "Building html documentation"
  )

  # doctest target
  set ( BUILDER doctest )
  configure_file ( runsphinx.py.in runsphinx_doctest.py @ONLY )
  add_custom_target ( ${TARGET_PREFIX}-test
    COMMAND ${PYTHON_EXECUTABLE} runsphinx_doctest.py
    COMMENT "Running documentation tests"
  )

endif ()


###############################################################################
# QtAssistant
###############################################################################
if ( APPLE )
  set ( ENABLE_QTASSISTANT False CACHE BOOL "Build qt-assistant documentation" )
else ()
  set ( ENABLE_QTASSISTANT True CACHE BOOL "Build qt-assistant documentation" )
endif ( APPLE )

if (ENABLE_QTASSISTANT)
  add_subdirectory ( qtassistant )
endif (ENABLE_QTASSISTANT)
