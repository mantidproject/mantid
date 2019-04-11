# This file manages building/installation of the mantidqt and mantidqtpython
# Python wrappers.
#
include ( PythonPackageTargetFunctions )

# Legacy wrappers for MantidPlot
add_subdirectory ( mantidqtpython )

###########################################################################
# mantidqt
###########################################################################

if ( ENABLE_WORKBENCH )
  # The default value is just an empty string - this prevents a Python syntax error when not on Windows
  # Configure utils.qt.plugins file for build. It is placed in the source
  # directory and added to the .gitignore for simplicity.
  if ( WIN32 )
    # Note: single quotes are added here to make this a Python string
    set ( QT_PLUGINS_PATH "${THIRD_PARTY_DIR}/lib/qt%V/plugins" )
  endif ()
  # Configure the file necessary for DEVELOPMENT builds to run and detect Qt's plugins
  configure_file ( mantidqt/utils/qt/plugins.py.in ${CMAKE_CURRENT_SOURCE_DIR}/mantidqt/utils/qt/plugins.py )
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
  add_subdirectory ( mantidqt )

  # Setup dependency chain
  add_dependencies ( mantidqt
    mantidqt_resources
    mantidqt_commonqt4
    mantidqt_commonqt5
    mantidqt_instrumentviewqt5
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

    mantidqt/dialogs/test/test_algorithm_dialog.py
    mantidqt/dialogs/test/test_spectraselectiondialog.py

    mantidqt/plotting/test/test_figuretype.py
    mantidqt/plotting/test/test_functions.py

    mantidqt/project/test/test_plotssaver.py
    mantidqt/project/test/test_plotsloader.py
    mantidqt/project/test/test_project.py
    mantidqt/project/test/test_projectloader.py
    mantidqt/project/test/test_projectsaver.py
    mantidqt/project/test/test_workspaceloader.py
    mantidqt/project/test/test_workspacesaver.py

    mantidqt/utils/test/test_async.py
    mantidqt/utils/test/test_modal_tester.py
    mantidqt/utils/test/test_qt_utils.py
    mantidqt/utils/test/test_writetosignal.py

    mantidqt/widgets/algorithmselector/test/test_algorithmselector.py
    mantidqt/widgets/codeeditor/test/test_codeeditor.py
    mantidqt/widgets/codeeditor/test/test_errorformatter.py
    mantidqt/widgets/codeeditor/test/test_execution.py
    mantidqt/widgets/codeeditor/test/test_interpreter.py
    mantidqt/widgets/codeeditor/test/test_multifileinterpreter.py
    mantidqt/widgets/workspacewidget/test/test_workspacetreewidget.py

    mantidqt/widgets/test/test_jupyterconsole.py
    mantidqt/widgets/test/test_messagedisplay.py
    mantidqt/widgets/test/test_fitpropertybrowser.py
    mantidqt/widgets/test/test_functionbrowser.py

    mantidqt/widgets/samplelogs/test/test_samplelogs_model.py
    mantidqt/widgets/samplelogs/test/test_samplelogs_presenter.py

    mantidqt/widgets/common/test/test_workspacedisplay_ads_observer.py
    mantidqt/widgets/common/test/test_observing_presenter.py
    mantidqt/widgets/common/test/test_observing_view.py

    mantidqt/widgets/matrixworkspacedisplay/test/test_matrixworkspacedisplay_model.py
    mantidqt/widgets/matrixworkspacedisplay/test/test_matrixworkspacedisplay_presenter.py
    mantidqt/widgets/matrixworkspacedisplay/test/test_matrixworkspacedisplay_table_view_model.py

    mantidqt/widgets/tableworkspacedisplay/test/test_tableworkspacedisplay_error_column.py
    mantidqt/widgets/tableworkspacedisplay/test/test_tableworkspacedisplay_marked_columns.py
    mantidqt/widgets/tableworkspacedisplay/test/test_tableworkspacedisplay_workbench_table_widget_item.py

    mantidqt/widgets/tableworkspacedisplay/test/test_tableworkspacedisplay_model.py
    mantidqt/widgets/tableworkspacedisplay/test/test_tableworkspacedisplay_presenter.py
  )

  # Tests
  set ( PYUNITTEST_RUN_SERIAL ON )
  set ( PYUNITTEST_QT_API pyqt5 )
  pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
    mantidqt_qt5 ${PYTHON_TEST_FILES}
  )
  # The jupyterconsole test segfaults on Python 3 and Qt 4
  # despite the code functioning in a normal usage.
  # Leave it out as it's a very basic test and the Qt5
  # tests should catch any problems anyway
  list ( REMOVE_ITEM PYTHON_TEST_FILES mantidqt/widgets/test/test_jupyterconsole.py )
  # The workspacewidget test randomly sefaults on exit with Qt4. It will not be used
  # in a Qt 4 context so remove the test
  list ( REMOVE_ITEM PYTHON_TEST_FILES mantidqt/widgets/workspacewidget/test/test_workspacetreewidget.py )
  set ( PYUNITTEST_QT_API pyqt )
  pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
    mantidqt_qt4 ${PYTHON_TEST_FILES}
  )
  unset ( PYUNITTEST_QT_API )
endif ()
