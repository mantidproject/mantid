###############################################################################
# Sphinx documentation
###############################################################################
find_package ( Sphinx 1.2 )

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

  # We generate a target per build type, i.e docs-html
  set ( SPHINX_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source )
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

  # qthelp target
  if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
    set ( BUILDER qthelp )
    set ( SPHINX_CONF_DIR ${SPHINX_BUILD_DIR}/conf/${BUILDER} )
    configure_file ( conf.py.in ${SPHINX_CONF_DIR}/conf.py @ONLY )
    configure_file ( runsphinx.py.in runsphinx_qthelp.py @ONLY )

    add_custom_command ( OUTPUT qthelp/MantidProject.qhcp
                                qthelp/MantidProject.qhp
                       COMMAND ${DOCS_RUNNER_EXE} -xq runsphinx_qthelp.py
                       DEPENDS Framework MantidPlot ${CMAKE_CURRENT_BINARY_DIR}/runsphinx_qthelp.py
                       COMMENT "Building qt-assistant index files")

    add_custom_command ( OUTPUT qthelp/MantidProject.qhc
                        COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} qthelp/MantidProject.qhcp
                        DEPENDS qthelp/MantidProject.qhcp
                        COMMENT "Compiling Qt help documentation")

    add_custom_target ( ${TARGET_PREFIX}-qthelp
                        DEPENDS qthelp/MantidProject.qhc ${SPHINX_CONF_DIR}/conf.py conf-qthelp.py )

    # Group within VS and exclude from whole build
    set_target_properties ( ${TARGET_PREFIX}-qthelp PROPERTIES FOLDER "Documentation"
                           EXCLUDE_FROM_DEFAULT_BUILD 1
                           EXCLUDE_FROM_ALL 1)

  else (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
    message (WARNING " Did not find qcollectiongenerator - cannot create Qt help files")
  endif ( QT_QCOLLECTIONGENERATOR_EXECUTABLE )

  # HTML target
  option (DOCS_HTML "Add target for building html docs" OFF)
  if (DOCS_HTML)
    set ( BUILDER html )
    set ( SPHINX_CONF_DIR ${SPHINX_BUILD_DIR}/conf/${BUILDER} )
    configure_file ( conf.py.in ${SPHINX_CONF_DIR}/conf.py @ONLY )
    configure_file ( runsphinx.py.in runsphinx_html.py @ONLY )
    add_custom_target ( ${TARGET_PREFIX}-html
                       COMMAND ${DOCS_RUNNER_EXE} -xq runsphinx_html.py
                       DEPENDS Framework MantidPlot ${CMAKE_CURRENT_BINARY_DIR}/runsphinx_html.py ${SPHINX_CONF_DIR}/conf.py conf-html.py
                       COMMENT "Building html documentation"
                       )
    # Group within VS and exclude from whole build
    set_target_properties ( ${TARGET_PREFIX}-html PROPERTIES FOLDER "Documentation"
                           EXCLUDE_FROM_DEFAULT_BUILD 1
                           EXCLUDE_FROM_ALL 1)
  endif (DOCS_HTML)

  ###############################################################################
  # Tests ( It would be nice to have this is a test sub directory but the
  #        configure of the runsphinx file doesn't like being in two directories.
  #        Ninja complains about multiple targets creating runsphinx.py.in)
  ###############################################################################

  ###############################################################################
  # Overall doctest target that executes in MantidPlot
  ###############################################################################
  set ( BUILDER doctest )
  set ( SPHINX_CONF_DIR ${SPHINX_BUILD_DIR}/conf/${BUILDER} )
  configure_file ( conf.py.in ${SPHINX_CONF_DIR}/conf.py @ONLY )
  configure_file ( runsphinx.py.in runsphinx_doctest.py @ONLY )
  add_custom_target ( ${TARGET_PREFIX}-test
    COMMAND ${DOCS_RUNNER_EXE} -xq runsphinx_doctest.py
    DEPENDS Framework MantidPlot ${CMAKE_CURRENT_BINARY_DIR}/runsphinx_doctest.py
            ${SPHINX_CONF_DIR}/conf.py
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
    if ( QT_QCOLLECTIONGENERATOR_EXECUTABLE )
     # must "make docs-qthelp" before "make package" otherwise there will be a build failure
     install (FILES ${SPHINX_BUILD_DIR}/qthelp/MantidProject.qhc
                    ${SPHINX_BUILD_DIR}/qthelp/MantidProject.qch
              DESTINATION ${INBUNDLE}${HTML_DOCS_DEST} )
     install ( DIRECTORY  ${SPHINX_BUILD_DIR}/qthelp/
              DESTINATION ${INBUNDLE}${HTML_DOCS_DEST}
              FILES_MATCHING PATTERN "*.html" )
     install ( DIRECTORY  ${SPHINX_BUILD_DIR}/qthelp/_images/
                          ${SPHINX_BUILD_DIR}/qthelp/_static/
              DESTINATION ${INBUNDLE}${HTML_DOCS_DEST} )
    else( QT_QCOLLECTIONGENERATOR_EXECUTABLE)
      install ( DIRECTORY ${SPHINX_BUILD_DIR}/html/
               DESTINATION ${INBUNDLE}${HTML_DOCS_DEST} )
    endif ( QT_QCOLLECTIONGENERATOR_EXECUTABLE )
  endif ( PACKAGE_DOCS )
else ( SPHINX_FOUND )
  message ( WARNING "Sphinx package (version >= 1.2) not found, unable to build documentation." )
endif ( SPHINX_FOUND )
