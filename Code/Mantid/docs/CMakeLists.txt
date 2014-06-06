###############################################################################
# Sphinx documentation
###############################################################################
find_package ( Sphinx )

if ( SPHINX_FOUND )
  # We generate a target per build type, i.e docs-html, docs-test
  set ( SPHINX_BUILD_DIR ${CMAKE_BINARY_DIR}/docs )
  set ( SCREENSHOTS_DIR ${SPHINX_BUILD_DIR}/screenshots )

  # targets
  set ( TARGET_PREFIX docs)
  # runner
  set ( DOCS_RUNNER_EXE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/MantidPlot )

  # HTML target
  set ( BUILDER html )
  configure_file ( runsphinx.py.in runsphinx_html.py @ONLY )
  add_custom_target ( ${TARGET_PREFIX}-html
    COMMAND ${DOCS_RUNNER_EXE} -xq runsphinx_html.py
    DEPENDS Framework MantidPlot
    COMMENT "Building html documentation"
  )
  # Group within VS and exclude from whole build
  set_target_properties ( ${TARGET_PREFIX}-html PROPERTIES FOLDER "Documentation"
                                                EXCLUDE_FROM_DEFAULT_BUILD 1
                                                EXCLUDE_FROM_ALL 1)

  # doctest target
  set ( BUILDER doctest )
  configure_file ( runsphinx.py.in runsphinx_doctest.py @ONLY )
  add_custom_target ( ${TARGET_PREFIX}-test
    COMMAND ${DOCS_RUNNER_EXE} -xq runsphinx_doctest.py
    DEPENDS Framework MantidPlot
    COMMENT "Running documentation tests"
  )
  # Group within VS and exclude from whole build
  set_target_properties ( ${TARGET_PREFIX}-test PROPERTIES FOLDER "Documentation"
                                                EXCLUDE_FROM_DEFAULT_BUILD 1
                                                EXCLUDE_FROM_ALL 1)
endif ()


###############################################################################
# QtAssistant
###############################################################################
if ( APPLE )
  set ( ENABLE_QTASSISTANT False CACHE BOOL "Build qt-assistant documentation" )
else ()
  set ( ENABLE_QTASSISTANT False CACHE BOOL "Build qt-assistant documentation" FORCE )
endif ( APPLE )

if (ENABLE_QTASSISTANT)
  add_subdirectory ( qtassistant )
endif (ENABLE_QTASSISTANT)
