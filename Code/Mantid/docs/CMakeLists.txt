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

  # qt-assistant target
  if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
    set ( BUILDER qthelp )
    configure_file ( runsphinx.py.in runsphinx_qthelp.py @ONLY )

    add_custom_command ( OUTPUT qthelp/MantidProject.qhcp
                                qthelp/MantidProject.qhp
                       COMMAND ${DOCS_RUNNER_EXE} -xq runsphinx_qthelp.py
                       DEPENDS Framework MantidPlot
                       COMMENT "Building qt-assistant index files")

    add_custom_command ( OUTPUT qthelp/MantidProject.qhc
                        COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} qthelp/MantidProject.qhcp
                        DEPENDS qthelp/MantidProject.qhcp
                        COMMENT "Compiling qt-assistant documentation")

    add_custom_target ( ${TARGET_PREFIX}-qthelp
                        DEPENDS qthelp/MantidProject.qhc )

    # Group within VS and exclude from whole build
    set_target_properties ( ${TARGET_PREFIX}-qthelp PROPERTIES FOLDER "Documentation"
                           EXCLUDE_FROM_DEFAULT_BUILD 1
                           EXCLUDE_FROM_ALL 1)

  else (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
    message (WARNING " Did not find qcollectiongenerator - cannot create qtassistant files")
  endif ( QT_QCOLLECTIONGENERATOR_EXECUTABLE )

  # HTML target
  option (DOCS_HTML "Add target for building html docs" OFF)
  if (DOCS_HTML)
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
  endif (DOCS_HTML)

  ###############################################################################
  # Tests ( It would be nice to have this is a test sub directory but the
  #        configure of the runsphinx file doesn't like being in two directories.
  #        Ninja complains about multiple targets creating runsphinx.py.in)
  #
  # Set up two things:
  #   - adds a global docs-test target to run all tests
  #   - adds individual tests through ctest for each rst file.
  ###############################################################################

  ###############################################################################
  # Overall doctest target that executes in MantidPlot
  ###############################################################################
  set ( BUILDER doctest )
  configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/runsphinx.py.in
                   ${CMAKE_CURRENT_BINARY_DIR}/runsphinx_doctest.py @ONLY )

  add_custom_target ( ${TARGET_PREFIX}-test
      COMMAND ${DOCS_RUNNER_EXE} -xq runsphinx_doctest.py
      DEPENDS Framework MantidPlot
      COMMENT "Running documentation tests in MantidPlot"
  )
  # Group within VS and exclude from whole build
  set_target_properties ( ${TARGET_PREFIX}-test PROPERTIES FOLDER "Documentation"
                                              EXCLUDE_FROM_DEFAULT_BUILD 1
                                              EXCLUDE_FROM_ALL 1)

  ###############################################################################
  # Add individual test targets for each file, if requested.
  # They will require the Framework target to be built
  ###############################################################################
  set ( SEPARATE_DOC_TESTS TRUE CACHE BOOL
       "If true, a separate test for each documentation file is added to ctest.")

  if( SEPARATE_DOC_TESTS )
    # The cmake documentation does not recommend using FILE (GLOB ) for populating
    # the list of files for a target: see GLOB section here -
    #   http://www.cmake.org/cmake/help/v2.8.8/cmake.html#command:file
    # However, adding hundreds of files in a list here seems equally bad from a maintenance
    # point of view as it will only get worse so we sacrifice not having to re-run cmake
    # occasionally when a non-code file is added for readability.

    file ( GLOB ALGDOC_TESTS RELATIVE ${SPHINX_SRC_DIR} ${SPHINX_SRC_DIR}/algorithms/*.rst )
    sphinx_add_test ( AlgorithmsDocTest ${CMAKE_CURRENT_BINARY_DIR}/runsphinx_doctest.py
                      ${ALGDOC_TESTS} )

  endif()

  ###############################################################################
  # Installation settings
  ###############################################################################
  # Allow control over whether the documentation is packaged
  set ( PACKAGE_DOCS False CACHE BOOL
        "If true the html documentation is installed in the share/doc directory of the package" )

  if ( PACKAGE_DOCS )
    set ( HTML_DOCS_DEST share/doc )
    if ( QT_QCOLLECTIONGENERATOR_EXECUTABLE )
     if (WIN32) # copy the assistant executable for windows 32 and 64
       set (QTASSISTANT_EXE ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/assistant.exe)
       add_custom_command( OUTPUT ${QTASSISTANT_EXE}
                           COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_LIBRARY_PATH}/assistant.exe
                           ${QTASSISTANT_EXE} )
     endif (WIN32)

     # must "make qtassistant" before "make package" otherwise there will be a build failure
     install (FILES ${SPHINX_BUILD_DIR}/qthelp/MantidProject.qhc
                    ${SPHINX_BUILD_DIR}/qthelp/MantidProject.qch
              DESTINATION ${HTML_DOCS_DEST} )
     install ( DIRECTORY  ${SPHINX_BUILD_DIR}/qthelp/
              DESTINATION ${HTML_DOCS_DEST}
              FILES_MATCHING PATTERN "*.html" )
     install ( DIRECTORY  ${SPHINX_BUILD_DIR}/qthelp/_images/
                          ${SPHINX_BUILD_DIR}/qthelp/_static/
              DESTINATION ${HTML_DOCS_DEST} )
    else( QT_QCOLLECTIONGENERATOR_EXECUTABLE)
      install ( DIRECTORY ${SPHINX_BUILD_DIR}/html
               DESTINATION ${HTML_DOCS_DEST} )
    endif ( QT_QCOLLECTIONGENERATOR_EXECUTABLE )
  endif ( PACKAGE_DOCS )
else ( SPHINX_FOUND )
  message ( WARNING "Sphinx package not found, unable to build documentation." )
endif ( SPHINX_FOUND )
