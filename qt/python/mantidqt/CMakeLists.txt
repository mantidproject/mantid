# This file manages building/installation of the mantidqt and mantidqtpython Python wrappers.

include(PythonPackageTargetFunctions)

# mantidqt
add_python_package(mantidqt)

# Now add any compiled sip targets
add_subdirectory(mantidqt)

# Setup dependency chain
if(BUILD_MANTIDFRAMEWORK)
  add_dependencies(mantidqt PythonInterface)
endif()

# Configure resources data in place for ease of development. The output file is added to the toplevel gitignore
set(_qrc_file ${CMAKE_CURRENT_LIST_DIR}/resources.qrc)
set(_output_res_py ${CMAKE_CURRENT_LIST_DIR}/mantidqt/resources.py)
configure_file(create_resources.cmake.in create_resources.cmake @ONLY)
add_custom_command(
  OUTPUT ${_output_res_py}
  COMMAND ${CMAKE_COMMAND} -P create_resources.cmake
  COMMENT "Generating mantidqt resources module"
  DEPENDS ${_qrc_file}
)
add_custom_target(mantidqt_resources ALL DEPENDS ${_output_res_py})

add_dependencies(mantidqt mantidqt_resources mantidqt_commonqt5 mantidqt_instrumentviewqt5 mantidqt_iconsqt5)
if(MSVC)
  # Debug builds need libraries that are linked with MSVC debug runtime
  add_custom_command(
    TARGET mantidqt
    POST_BUILD
    COMMAND
      if 1==$<CONFIG:Debug> ${CMAKE_COMMAND} -E copy_directory
      ${MSVC_PYTHON_EXECUTABLE_DIR}/msvc-site-packages/debug/PyQt5
      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/PyQt5
    COMMENT "Copying debug PyQt5 to bin/Debug"
  )
endif()

# Testing
add_dependencies(GUITests mantidqt)
# ctest targets
set(PYTHON_TEST_FILES
    mantidqt/test/test_algorithm_observer.py
    mantidqt/test/test_import.py
    mantidqt/dialogs/errorreports/test/test_errorreport_presenter.py
    mantidqt/dialogs/test/test_algorithm_dialog.py
    mantidqt/dialogs/test/test_spectraselectiondialog.py
    mantidqt/dialogs/test/test_spectraselectorutils.py
    mantidqt/plotting/test/test_figuretype.py
    mantidqt/plotting/test/test_functions.py
    mantidqt/plotting/test/test_tiledplots.py
    mantidqt/plotting/test/test_sample_shape.py
    mantidqt/project/test/test_plotssaver.py
    mantidqt/project/test/test_plotsloader.py
    mantidqt/project/test/test_project.py
    mantidqt/project/test/test_projectloader.py
    mantidqt/project/test/test_projectsaver.py
    mantidqt/project/test/test_workspaceloader.py
    mantidqt/project/test/test_workspacesaver.py
    mantidqt/project/test/test_projectparser_mantidplot.py
    mantidqt/utils/test/test_async.py
    mantidqt/utils/test/test_async_qt_adaptor.py
    mantidqt/utils/test/test_modal_tester.py
    mantidqt/utils/test/test_qt_utils.py
    mantidqt/utils/test/test_writetosignal.py
    mantidqt/utils/qt/test/test_line_edit_double_validator.py
    mantidqt/utils/qt/test/test_qappthreadcall.py
    mantidqt/widgets/test/test_messagedisplay.py
    mantidqt/widgets/test/test_scriptrepository.py
    mantidqt/widgets/test/test_fittingmode.py
    mantidqt/widgets/test/test_fitpropertybrowser.py
    mantidqt/widgets/test/test_fitpropertybrowserbase.py
    mantidqt/widgets/test/test_fitpropertybrowserplotinteraction.py
    mantidqt/widgets/test/test_functionbrowser.py
    mantidqt/widgets/samplelogs/test/test_samplelogs_model.py
    mantidqt/widgets/samplelogs/test/test_samplelogs_presenter.py
    mantidqt/widgets/observers/test/test_ads_observer.py
    mantidqt/widgets/observers/test/test_observing_presenter.py
    mantidqt/widgets/observers/test/test_observing_view.py
    mantidqt/widgets/embedded_find_replace_dialog/test/test_embedded_find_replace_dialog_presenter.py
    mantidqt/widgets/waterfallplotfillareadialog/test/test_waterfallplotfillareadialogpresenter.py
    mantidqt/widgets/waterfallplotoffsetdialog/test/test_waterfallplotoffsetdialogpresenter.py
)

# ctest target for widgets that only get tested in qt5, because they are only used in the workbench
set(PYTHON_WIDGET_QT5_ONLY_TESTS
    mantidqt/widgets/algorithmselector/test/test_algorithm_factory_observer.py
    mantidqt/widgets/algorithmselector/test/test_algorithmselector.py
    mantidqt/widgets/codeeditor/test/test_codecommenter.py
    mantidqt/widgets/codeeditor/test/test_codeeditor.py
    mantidqt/widgets/codeeditor/test/test_completion.py
    mantidqt/widgets/codeeditor/test/test_execution.py
    mantidqt/widgets/codeeditor/test/test_errorformatter.py
    mantidqt/widgets/codeeditor/test/test_interpreter.py
    mantidqt/widgets/codeeditor/test/test_interpreter_view.py
    mantidqt/widgets/codeeditor/test/test_multifileinterpreter.py
    mantidqt/widgets/codeeditor/test/test_multifileinterpreter_view.py
    mantidqt/widgets/codeeditor/test/test_scriptcompatibility.py
    mantidqt/widgets/codeeditor/tab_widget/test/test_codeeditor_tab_presenter.py
    mantidqt/widgets/codeeditor/tab_widget/test/test_codeeditor_tab_view.py
    mantidqt/widgets/colorbar/test/test_colorbar.py
    mantidqt/widgets/fitpropertybrowser/addfunctiondialog/test/test_addfunctiondialogpresenter.py
    mantidqt/widgets/fitpropertybrowser/addfunctiondialog/test/test_addfunctiondialogview.py
    mantidqt/widgets/fitpropertybrowser/test/test_interactive_tool.py
    mantidqt/widgets/instrumentview/test/test_instrumentview_io.py
    mantidqt/widgets/instrumentview/test/test_instrumentview_presenter.py
    mantidqt/widgets/instrumentview/test/test_instrumentview_view.py
    mantidqt/widgets/memorywidget/test/test_memoryinfo.py
    mantidqt/widgets/memorywidget/test/test_memorypresenter.py
    mantidqt/widgets/plotconfigdialog/axestabwidget/test/test_axestabwidgetpresenter.py
    mantidqt/widgets/plotconfigdialog/curvestabwidget/test/test_curveproperties.py
    mantidqt/widgets/plotconfigdialog/curvestabwidget/test/test_curvestabwidgetpresenter.py
    mantidqt/widgets/plotconfigdialog/curvestabwidget/test/test_markertabwidget_view.py
    mantidqt/widgets/plotconfigdialog/test/test_apply_all_properties.py
    mantidqt/widgets/plotconfigdialog/imagestabwidget/test/test_imagestabwidgetpresenter.py
    mantidqt/widgets/plotconfigdialog/test/test_plotconfigdialogpresenter.py
    mantidqt/widgets/samplelogs/test/test_samplelogs_view.py
    mantidqt/widgets/samplematerialdialog/test/test_samplematerial_presenter.py
    mantidqt/widgets/regionselector/test/test_regionselector_presenter.py
    mantidqt/widgets/sliceviewer/test/test_base_presenter.py
    mantidqt/widgets/sliceviewer/test/test_dimensions.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_cursortracker.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_imageinfowidget.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_lineplots.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_model.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_movemousecursor.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_presenter.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_sliceinfo.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_transform.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_dataview.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_dimensionwidget.py
    mantidqt/widgets/sliceviewer/test/test_sliceviewer_zoom.py
    mantidqt/widgets/sliceviewer/test/test_workspace_info.py
    mantidqt/widgets/sliceviewer/test/test_roi.py
    mantidqt/widgets/sliceviewer/cutviewer/test/test_cutviewer_model.py
    mantidqt/widgets/sliceviewer/cutviewer/test/test_cutviewer_presenter.py
    mantidqt/widgets/sliceviewer/cutviewer//representation/test/test_cut_representation.py
    mantidqt/widgets/sliceviewer/peaksviewer/test/test_peaksviewer_actions.py
    mantidqt/widgets/sliceviewer/peaksviewer/test/test_peaksviewer_model.py
    mantidqt/widgets/sliceviewer/peaksviewer/test/test_peaksviewer_presenter.py
    mantidqt/widgets/sliceviewer/peaksviewer/test/test_peaksviewer_view.py
    mantidqt/widgets/sliceviewer/peaksviewer/test/test_peaksviewer_peaksviewercollectionpresenter.py
    mantidqt/widgets/sliceviewer/peaksviewer/test/test_peaksviewer_peaksworkspaceselectormodel.py
    mantidqt/widgets/sliceviewer/peaksviewer/test/test_peaksviewer_peaksworkspaceselectorpresenter.py
    mantidqt/widgets/sliceviewer/peaksviewer/test/test_peaksviewer_sliceviewer_add_delete_peaks_integration.py
    mantidqt/widgets/sliceviewer/peaksviewer/representation/test/test_peaksviewer_representation_alpha.py
    mantidqt/widgets/sliceviewer/peaksviewer/representation/test/test_peaksviewer_representation_draw.py
    mantidqt/widgets/sliceviewer/peaksviewer/representation/test/test_peaksviewer_representation_painter.py
    mantidqt/widgets/sliceviewer/peaksviewer/representation/test/test_peaksviewer_representation_nonintegrated.py
    mantidqt/widgets/sliceviewer/peaksviewer/representation/test/test_peaksviewer_representation_ellipsoid.py
    mantidqt/widgets/superplot/test/test_superplot_model.py
    mantidqt/widgets/superplot/test/test_superplot_presenter.py
    mantidqt/widgets/superplot/test/test_superplot_view.py
    mantidqt/widgets/test/test_jupyterconsole.py
    mantidqt/widgets/workspacecalculator/test/test_workspacecalculator_view.py
    mantidqt/widgets/workspacecalculator/test/test_workspacecalculator_presenter.py
    mantidqt/widgets/workspacecalculator/test/test_workspacecalculator_model.py
    mantidqt/widgets/workspacedisplay/test/test_data_copier.py
    mantidqt/widgets/workspacedisplay/test/test_workspacedisplay_view.py
    mantidqt/widgets/workspacedisplay/test/test_user_notifier.py
    mantidqt/widgets/workspacedisplay/matrix/test/test_matrixworkspacedisplay_delegate.py
    mantidqt/widgets/workspacedisplay/matrix/test/test_matrixworkspacedisplay_model.py
    mantidqt/widgets/workspacedisplay/matrix/test/test_matrixworkspacedisplay_presenter.py
    mantidqt/widgets/workspacedisplay/matrix/test/test_matrixworkspacedisplay_table_view_model.py
    mantidqt/widgets/workspacedisplay/matrix/test/test_matrixworkspacedisplay_view.py
    mantidqt/widgets/workspacedisplay/matrix/test/test_matrixworkspacedisplay_io.py
    mantidqt/widgets/workspacedisplay/table/test/test_tableworkspacedisplay_error_column.py
    mantidqt/widgets/workspacedisplay/table/test/test_tableworkspacedisplay_group_model.py
    mantidqt/widgets/workspacedisplay/table/test/test_tableworkspacedisplay_group_table_model.py
    mantidqt/widgets/workspacedisplay/table/test/test_tableworkspacedisplay_marked_columns.py
    mantidqt/widgets/workspacedisplay/table/test/test_tableworkspacedisplay_tableworkspace_item.py
    mantidqt/widgets/workspacedisplay/table/test/test_tableworkspacedisplay_model.py
    mantidqt/widgets/workspacedisplay/table/test/test_tableworkspacedisplay_presenter.py
    mantidqt/widgets/workspacedisplay/table/test/test_tableworkspacedisplay_table_model.py
    mantidqt/widgets/workspacedisplay/table/test/test_tableworkspacedisplay_view.py
    mantidqt/widgets/workspacedisplay/table/test/test_tableworkspacedisplay_io.py
    mantidqt/widgets/workspacewidget/test/test_workspacetreewidget.py
    mantidqt/icons/test/test_icons.py
)

# Tests
set(PYUNITTEST_QT_API pyqt5)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} mantidqt_qt5 ${PYTHON_TEST_FILES} ${PYTHON_WIDGET_QT5_ONLY_TESTS})
