# This file manages building/installation of the mantidqt and mantidqtpython
# Python wrappers.
#

# Legacy wrappers for MantidPlot
add_subdirectory ( mantidqtpython )

###########################################################################
# mantidqt
###########################################################################

if ( ENABLE_WORKBENCH )

  # Configure utils.qt.plugins file for build. It is placed in the source
  # directory and added to the .gitignore for simplicity.
  if ( WIN32 )
    set ( QT_PLUGINS_PATH "${THIRD_PARTY_DIR}/lib/qt%V/plugins" )
  endif ()
    configure_file ( mantidqt/utils/qt/plugins.py.in
      ${CMAKE_CURRENT_SOURCE_DIR}/mantidqt/utils/qt/plugins.py
  )

  # Create egg link to binary output directory for mantidqt
  add_python_package ( mantidqt )

  # Configure resources data in place for ease of development. The output
  # file is added to the toplevel gitignore
  set ( _qrc_file ${CMAKE_CURRENT_LIST_DIR}/resources.qrc )
  set ( _output_res_py ${CMAKE_CURRENT_LIST_DIR}/mantidqt/resources.py )
  configure_file ( create_resources.cmake.in create_resources.cmake @ONLY)
  add_custom_command ( OUTPUT ${_output_res_py}
    COMMAND ${CMAKE_COMMAND} -P create_resources.cmake
    COMMENT "Generating mantidqt resources module"
    DEPENDS ${_qrc_file}
  )
  add_custom_target ( mantidqt_resources ALL DEPENDS ${_output_res_py} )

  # Now add any compiled sip targets
  add_subdirectory ( mantidqt/widgets )

  # Setup dependency chain
  add_dependencies ( mantidqt
    mantidqt_widgetscoreqt5
    mantidqt_widgetscoreqt4
    mantidqt_resources
  )

  if ( MSVC )
    # Debug builds need libraries that are linked with MSVC debug runtime
    add_custom_command (
      TARGET mantidqt
      POST_BUILD
      COMMAND if 1==$<CONFIG:Debug> ${CMAKE_COMMAND}
        -E copy_directory ${PYTHON_DIR}/msvc-site-packages/debug/PyQt5 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/PyQt5
      COMMENT "Copying debug PyQt5 to bin/Debug"
    )
endif ()


  ##########################################################################
  # Testing
  ##########################################################################

  # ctest targets
  set ( PYTHON_TEST_FILES
    mantidqt/test/test_algorithm_observer.py
    mantidqt/test/test_import.py

    mantidqt/utils/test/test_async.py
    mantidqt/utils/test/test_modal_tester.py
    mantidqt/utils/test/test_qt_utils.py
    mantidqt/utils/test/test_writetosignal.py

    mantidqt/widgets/test/test_algorithm_dialog.py

    mantidqt/widgets/codeeditor/test/test_codeeditor.py
    mantidqt/widgets/codeeditor/test/test_errorformatter.py
    mantidqt/widgets/codeeditor/test/test_execution.py
    mantidqt/widgets/codeeditor/test/test_interpreter.py
    mantidqt/widgets/codeeditor/test/test_multifileinterpreter.py

    mantidqt/widgets/workspacewidget/test/test_plotselectiondialog.py

    mantidqt/widgets/test/test_algorithmselector.py
    mantidqt/widgets/test/test_jupyterconsole.py
    mantidqt/widgets/test/test_messagedisplay.py
  )

  # Tests
  pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
    mantidqt ${PYTHON_TEST_FILES}
  )

  # No package installation yet...
  # Configure utils.qt.plugins file for install
endif ()
