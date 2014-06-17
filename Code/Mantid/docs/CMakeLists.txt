###############################################################################
# Sphinx documentation
###############################################################################
find_package ( Sphinx )

if ( SPHINX_FOUND )
  # run python to see if the theme is installed
  execute_process ( COMMAND ${PYTHON_EXECUTABLE} -c "import sphinx_bootstrap_theme"
                    OUTPUT_VARIABLE SPHINX_BOOTSTRAP_THEME_OUT
                    ERROR_VARIABLE SPHINX_BOOTSTRAP_THEME_ERR
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_STRIP_TRAILING_WHITESPACE )

  # require the bootstrap theme
  if (SPHINX_BOOTSTRAP_THEME_ERR)
    message (ERROR " Did not find sphinx_bootstrap_theme")
    message (STATUS "${PYTHON_EXECUTABLE} -c \"import sphinx_bootstrap_theme\"")
    message (STATUS "${SPHINX_BOOTSTRAP_THEME_ERR}")
    message (FATAL_ERROR " Install instructions at https://pypi.python.org/pypi/sphinx-bootstrap-theme/")
  endif (SPHINX_BOOTSTRAP_THEME_ERR)

  # We generate a target per build type, i.e docs-html, docs-test
  set ( SPHINX_BUILD_DIR ${DOCS_BUILDDIR} )
  set ( SCREENSHOTS_DIR ${SPHINX_BUILD_DIR}/screenshots )

  # targets
  set ( TARGET_PREFIX docs)
  # runner
  if ( APPLE )
    set ( DOCS_RUNNER_EXE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${BIN_DIR}/MantidPlot )
  else ()
    set ( DOCS_RUNNER_EXE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/MantidPlot )
  endif()

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

  ###############################################################################
  # Installation settings
  ###############################################################################
  # Allow control over whether the documentation is packaged
  set ( PACKAGE_DOCS False CACHE BOOL
        "If true the html documentation is installed in the share/doc directory of the package" )

  if ( PACKAGE_DOCS )
    set ( HTML_DOCS_DEST share/doc )
    install ( DIRECTORY ${SPHINX_BUILD_DIR}/html
              DESTINATION ${HTML_DOCS_DEST} )
  endif ()
else ()
  message ( WARNING "Sphinx package not found, unable to build documentation." )
endif ()
