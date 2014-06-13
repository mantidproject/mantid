###############################################################################
# Sphinx documentation
###############################################################################
find_package ( Sphinx )

if ( SPHINX_FOUND )
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

  if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
    # qt-assistant target
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

  endif (QT_QCOLLECTIONGENERATOR_EXECUTABLE)

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
