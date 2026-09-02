# First generate rules for the workbench application itself egg link name has to match that in setup.py name or Visual
# Studio generates a warning that the output filename of the target does not match the output that is produced by
# setuptools.
if(APPLE OR WIN32)
  if(WIN32)
    # handled separately below
    set(_exclude_on_install "resources\.pyc?")
  endif()
  add_python_package(workbench EXECUTABLE EGGLINKNAME MantidWorkbench EXCLUDE_FROM_INSTALL ${_exclude_on_install})
  # Allow VS to start workbench for debugging
  set(_vs_debugger_args "-m workbench --single-process")
  # cmake-format: off
  set_target_properties(
    workbench
    PROPERTIES VS_DEBUGGER_COMMAND $<$<BOOL:${WIN32}>:${Python_EXECUTABLE}>
               VS_DEBUGGER_COMMAND_ARGUMENTS $<$<BOOL:${WIN32}>:${_vs_debugger_args}>
               VS_DEBUGGER_ENVIRONMENT
               $<$<BOOL:${WIN32}>:${MSVC_IDE_ENV}>
  )
  # cmake-format: on
else()
  add_python_package(
    workbench EXECUTABLE
    EGGLINKNAME MantidWorkbench
    INSTALL_LIB_DIRS "${WORKBENCH_SITE_PACKAGES}"
    INSTALL_BIN_DIR "${WORKBENCH_BIN_DIR}"
  )
endif()

# Resource files. PyQt6 has no Python resource compiler (pyrcc was removed in Qt6), so resources.py cannot be generated.
# Instead compile resources.qrc to a binary .rcc with Qt's own rcc (QT_RCC_CMD, set in qt/CMakeLists.txt) and register
# it at runtime via QResource.registerResource - see workbench/app/resource_loader.py.
set(_images_qrc_file ${CMAKE_CURRENT_LIST_DIR}/resources.qrc)
if(CONDA_BUILD)
  set(_output_rcc ${WORKBENCH_SITE_PACKAGES}/workbench/app/workbench.rcc)
else()
  set(_output_rcc ${CMAKE_CURRENT_LIST_DIR}/workbench/app/workbench.rcc)
endif()
if(QT_RCC_CMD)
  add_custom_command(
    OUTPUT ${_output_rcc}
    COMMAND ${QT_RCC_CMD} --binary -o ${_output_rcc} ${_images_qrc_file}
    COMMENT "Generating workbench binary resources (Qt${MANTID_QT_VERSION}): ${_output_rcc}"
    DEPENDS ${_images_qrc_file}
  )
  add_custom_target(workbench_resources ALL DEPENDS ${_output_rcc})
else()
  message(STATUS "QT_RCC_CMD not set (Qt${MANTID_QT_VERSION}): skipping workbench resource compilation")
  add_custom_target(workbench_resources ALL)
endif()

# Dependency chain
add_dependencies(workbench workbench_resources)
# if we are building mantidqt we can add a dependency for workbench
if(MANTID_QT_LIB STREQUAL "BUILD")
  add_dependencies(workbench mantidqtinterfaces instrumentview)
endif()

# Testing
set(TEST_FILES
    workbench/config/test/test_user.py
    workbench/test/test_mainwindow.py
    workbench/test/test_import.py
    workbench/test/test_workbench_process.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratoraxes.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorcolorfills.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorfigure.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorlegend.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorlines.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorutils.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorfitting.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgenerator.py
    workbench/plotting/test/test_figureerrorsmanager.py
    workbench/plotting/test/test_figureinteraction.py
    workbench/plotting/test/test_figuremanager.py
    workbench/plotting/test/test_figurewindow.py
    workbench/plotting/test/test_globalfiguremanager.py
    workbench/plotting/test/test_mantidfigurecanvas.py
    workbench/plotting/test/test_propertiesdialog.py
    workbench/plotting/test/test_plothelppages.py
    workbench/plotting/test/test_toolbar.py
    workbench/plotting/test/test_utility.py
    workbench/plugins/test/test_editor.py
    workbench/plugins/test/test_exception_handler.py
    workbench/plugins/test/test_jupyterconsole.py
    workbench/plugins/test/test_workspacewidget.py
    workbench/utils/test/test_workspacehistorygeneration.py
    workbench/widgets/about/test/test_about_presenter.py
    workbench/widgets/plotselector/test/test_plotselector_model.py
    workbench/widgets/plotselector/test/test_plotselector_presenter.py
    workbench/widgets/plotselector/test/test_plotselector_view.py
    workbench/widgets/settings/test/test_settings_model.py
    workbench/widgets/settings/test/test_settings_presenter.py
    workbench/widgets/settings/test/test_settings_view.py
    workbench/widgets/settings/base_classes/test/test_config_settings_changes_model.py
    workbench/widgets/settings/base_classes/test/test_config_settings_presenter.py
    workbench/widgets/settings/general/test/test_general_settings.py
    workbench/widgets/settings/general/test/test_general_settings_model.py
    workbench/widgets/settings/plots/test/test_plot_settings.py
    workbench/widgets/settings/plots/test/test_plot_settings_model.py
    workbench/widgets/settings/categories/test/test_categories_settings.py
    workbench/widgets/settings/categories/test/test_categories_settings_model.py
    workbench/widgets/settings/fitting/test/test_fitting_settings.py
    workbench/widgets/settings/fitting/test/test_fitting_settings_model.py
    workbench/projectrecovery/test/test_projectrecovery.py
    workbench/projectrecovery/test/test_projectrecoveryloader.py
    workbench/projectrecovery/test/test_projectrecoverysaver.py
    workbench/projectrecovery/recoverygui/test/test_projectrecoverymodel.py
    workbench/projectrecovery/recoverygui/test/test_projectrecoverypresenter.py
    workbench/projectrecovery/recoverygui/test/test_projectrecoverywidgetview.py
    workbench/projectrecovery/recoverygui/test/test_recoveryfailureview.py
)

set(PYUNITTEST_RUN_SERIAL ON)
set(PYUNITTEST_QT_API pyqt${MANTID_QT_VERSION})
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} workbench ${TEST_FILES})
